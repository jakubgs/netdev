#include <linux/slab.h>     /* kmalloc, kzalloc, kfree and so on */
#include <linux/device.h> 

#include "netdevmgm.h"
#include "fo.h"

static DEFINE_HASHTABLE(netdev_htable, NETDEV_HASHTABLE_SIZE);
static struct rw_semaphore netdev_htable_sem;
unsigned int netdev_count;
struct netdev_data **netdev_devices;

struct netdev_data * ndmgm_alloc_data(int nlpid, char *name) {
    int err = 0;
    struct netdev_data *nddata; /* TODO needs to be in a list */

    /* check if we have space for another device */
    /* TODO needs a reuse of device numbers from netdev_devices array */
    if ( netdev_count + 1 > NETDEV_MAX_DEVICES ) {
        printk(KERN_ERR "netdev_create: max devices reached = %d\n",
                        NETDEV_MAX_DEVICES);
        return 0;
    }

    /* GFP_KERNEL means this function can be blocked,
     * so it can't be part of an atomic operation.
     * For that GFP_ATOMIC would have to be used. */
    nddata = (struct netdev_data *) kzalloc(
                                sizeof(struct netdev_data), GFP_KERNEL);
    if (!nddata) {
        printk(KERN_ERR "netdev_create: failed to allocate nddata\n");
        return NULL;
    }

    nddata->cdev = (struct cdev *) kzalloc(
                                sizeof(struct cdev), GFP_KERNEL);
    if (!nddata->cdev) {
        printk(KERN_ERR "netdev_create: failed to allocate nddata->cdev\n");
        goto free_nddata;
    }

    if ((err = kfifo_alloc(&nddata->fo_queue, NETDEV_FO_QUEUE_SIZE, GFP_KERNEL))) {
        printk(KERN_ERR "netdev_create: failed to allocate fo_queue\n");
        goto free_cdev;
    }

    nddata->queue_pool = kmem_cache_create(NETDEV_REQ_POOL_NAME,
                        sizeof(struct fo_req),
                        0, 0, NULL); /* no alignment, flags or constructor */
    if (!nddata->queue_pool) {
        printk(KERN_ERR "netdev_create: failed to allocate queue_pool\n");
        goto free_fo_queue;
    }

    init_rwsem(&nddata->sem);
    nddata->nlpid = nlpid;
    nddata->devname = name;
    nddata->active = true;

    return nddata;
free_fo_queue:
    kfifo_free(&nddata->fo_queue);
free_cdev:
    kfree(nddata->cdev);
free_nddata:
    kfree(nddata);
    return NULL;
}

int ndmgm_free_data(struct netdev_data *nddata) {
    int size = 0;
    struct fo_req *req = NULL;

    /* destroy all requests in the queue */
    while (kfifo_avail(&nddata->fo_queue)) {
        size = kfifo_out(&nddata->fo_queue, &req, sizeof(struct fo_req *));
        if ( size != sizeof(struct fo_req *) ) {
            printk(KERN_ERR "netdev_destroy: failed to fetch from queue\n");
            continue;
        }

        req->rvalue = -ENODATA;
        complete(&req->comp); /* complete all pending file operations */

        kmem_cache_free(nddata->queue_pool, req);
    }

    kmem_cache_destroy(nddata->queue_pool);
    kfifo_free(&nddata->fo_queue);
    kfree(nddata->cdev);
    kfree(nddata);

    return 0; /* success */
}

/*  TODO all this shit will HAVE to use semafors if it has to work */
int ndmgm_create(int nlpid, char *name) {
    int err = 0;
    struct netdev_data *nddata = NULL;
    printk(KERN_DEBUG "netdev_create: creating device \\dev\\%s%d\n",
                        name,
                        netdev_count);

    if ( (nddata = ndmgm_alloc_data(nlpid, name)) == NULL ) {
        printk(KERN_ERR "netdev_create: failed to create netdev_data\n");
        return -ENOMEM;
    }

    cdev_init(nddata->cdev, &netdev_fops);
    nddata->cdev->owner = THIS_MODULE;
    nddata->cdev->dev = MKDEV(MAJOR(netdev_devno), netdev_count);

    /* tell the kernel the cdev structure is ready,
     * if it is not do not call cdev_add */
    err = cdev_add(nddata->cdev, nddata->cdev->dev, 1);
    printk(KERN_DEBUG "netdev_create: TEST!\n");

    /* Unlikely but might fail */
    if (unlikely(err)) {
        printk(KERN_ERR "Error %d adding netdev\n", err);
        goto free_nddata;
    }

    nddata->device = device_create(netdev_class,
                    NULL,              /* no aprent device           */
                    nddata->cdev->dev, /* major and minor numbers    */
                    nddata,            /* device data for callback   */
                    "%s%d",            /* defines name of the device */
                    name,
                    netdev_count);

    if (IS_ERR(nddata->device)) {
       err = PTR_ERR(nddata->device);
       printk(KERN_WARNING "[target] Error %d while trying to name %s%d\n",
                            err,
                            name,
                            netdev_count);
       goto undo_cdev;
    }

    printk(KERN_DEBUG "netdev: new device: %d, %d\n",
                        MAJOR(nddata->cdev->dev),
                        MINOR(nddata->cdev->dev));

    netdev_count++;
    /* add the device to hashtable with all devices since it's ready */
    hash_add(netdev_htable, &nddata->hnode, (int)nlpid);
    netdev_devices[MINOR(nddata->cdev->dev)] = nddata;

    return 1;
undo_cdev:
    cdev_del(nddata->cdev);
fail:
    netdev_destroy(nlpid); /* TODO fix this shit */
    return 0;
}

int ndmgm_find_destroy(int nlpid) {
    struct netdev_data *nddata = NULL;

    nddata = ndmgm_find(nlpid);

    if (down_read_trylock(&netdev_htable_sem)) {
        hash_del(&nddata->hnode);
        netdev_devices[MINOR(nddata->cdev->dev)] = NULL;

        up_read(&netdev_htable_sem);
    }

    return ndmgm_destroy(nddata);
}

int ndmgm_destroy(struct netdev_data *nddata) {
    if (nddata) {
        if (down_write_trylock(&nddata->sem)) {
            device_destroy(netdev_class, nddata->cdev->dev);
            cdev_del(nddata->cdev);
            /* kfree can take null as argument, no test needed */
            kfree(nddata->cdev);
            kfree(nddata);
            
            up_write(&nddata->sem);
            return 0; /* success */
        }
    }

    return 1;
}

int ndmgm_end(void) {
    int i = 0;
    struct netdev_data *nddata = NULL;
    struct hlist_node *tmp;

    if (down_write_trylock(&netdev_htable_sem)) {
        /* needs to be _safe so we can delete elements inside the loop */
        hash_for_each_safe(netdev_htable, i, tmp, nddata, hnode) {
            printk(KERN_DEBUG "netdev_end: deleting dev pid = %d\n",
                                nddata->nlpid);
            hash_del(&nddata->hnode); /* delete the element from table */
            netdev_devices[MINOR(nddata->cdev->dev)] = NULL;

            if (!ndmgm_destroy(nddata)) {
                printk(KERN_ERR "netdev_end: failed to destroy nddata = %d\n",
                                nddata->nlpid);
            }
        }
        up_write(&netdev_htable_sem);
        return 0; /* success */
    }
    return 1; /* failure */
}

void ndmgm_prepare(void) {
    /* create and array for all drivices which will be indexed with
     * minor numbers of those devices */
    netdev_devices = (struct netdev_data**)kcalloc(NETDEV_MAX_DEVICES,
                                            sizeof(struct netdev_data*),
                                            GFP_KERNEL);
    /* create the hashtable which will store data about created devices
     * and for easy access through pid */
    hash_init(netdev_htable);
    netdev_count = 0;
}

struct netdev_data* ndmgm_find(int nlpid) {
    struct netdev_data *nddata = NULL;

    if (down_read_trylock(&netdev_htable_sem)) {
        hash_for_each_possible(netdev_htable, nddata, hnode, (int)nlpid) {
            if ( nddata->nlpid == nlpid ) {
                up_read(&netdev_htable_sem);
                return nddata;
            }
        }
        up_read(&netdev_htable_sem);
    }

    return NULL;
}

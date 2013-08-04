#include <linux/slab.h>     /* kmalloc, kzalloc, kfree and so on */
#include <linux/device.h> 

#include "netdevmgm.h"
#include "dbg.h"

static DEFINE_HASHTABLE(netdev_htable, NETDEV_HASHTABLE_SIZE);
static struct rw_semaphore netdev_htable_sem;
unsigned int netdev_count;
int *netdev_minors_used;

struct netdev_data * ndmgm_alloc_data(int nlpid, char *name)
{
    int err = 0;
    struct netdev_data *nddata; /* TODO needs to be in a list */

    /* check if we have space for another device */
    /* TODO needs a reuse of device numbers from netdev_minors_used array */
    if ( netdev_count + 1 > NETDEV_MAX_DEVICES ) {
        printk(KERN_ERR "netdev_create: max devices reached = %d\n",
                        NETDEV_MAX_DEVICES);
        return 0;
    }

    /* GFP_KERNEL means this function can be blocked,
     * so it can't be part of an atomic operation.
     * For that GFP_ATOMIC would have to be used. */
    nddata = kzalloc(sizeof(*nddata), GFP_KERNEL);
    if (!nddata) {
        printk(KERN_ERR "netdev_create: failed to allocate nddata\n");
        return NULL;
    }

    nddata->devname = kzalloc(strlen(name), GFP_KERNEL);
    if (!nddata->devname) {
        printk(KERN_ERR "netdev_create: failed to allocate nddata\n");
        goto free_devname;
    }

    nddata->cdev = kzalloc(sizeof(*nddata->cdev), GFP_KERNEL);
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
    sprintf(nddata->devname, "/dev/%s%d", name, netdev_count);
    nddata->nlpid = nlpid;
    nddata->active = true;

    return nddata;
free_fo_queue:
    kfifo_free(&nddata->fo_queue);
free_cdev:
    kfree(nddata->cdev);
free_devname:
    kfree(nddata->devname);
free_nddata:
    kfree(nddata);
    return NULL;
}

void ndmgm_free_data(struct netdev_data *nddata)
{
    kmem_cache_destroy(nddata->queue_pool);
    kfifo_free(&nddata->fo_queue);
    kfree(nddata->cdev);
    kfree(nddata->devname);
    kfree(nddata);
}

int ndmgm_free_queue(struct netdev_data *nddata)
{
    int size = 0;
    struct fo_req *req = NULL;

    /* destroy all requests in the queue */
    while (!kfifo_is_empty(&nddata->fo_queue)) {
        printk(KERN_ERR "netdev_destroy: queue not emtpy\n");
        size = kfifo_out(&nddata->fo_queue, &req, sizeof(req));
        if ( size != sizeof(req) || IS_ERR(req)) {
            printk(KERN_ERR "netdev_destroy: failed to fetch from queue, size = %d\n", size);
            return 1; /* failure */
        }

        req->rvalue = -ENODATA;
        complete(&req->comp); /* complete all pending file operations */

        kmem_cache_free(nddata->queue_pool, req);
    }

    return 0; /* success */
}

/* this function safely increases the current sequence number and returns it,
 * 0 value is restricted since it's used to indicate failure */
int ndmgm_incseq(struct netdev_data *nddata)
{
    int seq = 0; /* failure */

    if (down_write_trylock(&nddata->sem)) {
        seq = nddata->curseq++;

        if ( seq == 0 ) { /* 0 is not a valide sequence number */
            seq = nddata->curseq++;
        }

        up_write(&nddata->sem);
    }

    return seq;
}

/*  TODO all this shit will HAVE to use semafors if it has to work */
int ndmgm_create(int nlpid, char *name)
{
    int err = 0;
    struct netdev_data *nddata = NULL;
    debug("creating device /dev/%s%d",
                        name,
                        netdev_count);

    if ( (nddata = ndmgm_alloc_data(nlpid, name)) == NULL ) {
        printk(KERN_ERR "netdev_create: failed to create netdev_data\n");
        return 1;
    }
    debug("nddata = %p", nddata);

    cdev_init(nddata->cdev, &netdev_fops);
    nddata->cdev->owner = THIS_MODULE;
    nddata->cdev->dev = MKDEV(MAJOR(netdev_devno), netdev_count);
    nddata->dummy = true; /* should be false for a server device */

    /* tell the kernel the cdev structure is ready,
     * if it is not do not call cdev_add */
    err = cdev_add(nddata->cdev, nddata->cdev->dev, 1);

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
                            MINOR(nddata->cdev->dev));

    if (IS_ERR(nddata->device)) {
       err = PTR_ERR(nddata->device);
       printk(KERN_WARNING "[target] Error %d while trying to name %s%d\n",
                            err,
                            name,
                            MINOR(nddata->cdev->dev));
       goto undo_cdev;
    }

    debug("new device: %d, %d",
                        MAJOR(nddata->cdev->dev),
                        MINOR(nddata->cdev->dev));

    netdev_count++;
    /* add the device to hashtable with all devices since it's ready */
    ndmgm_get(nddata); /* increase count for hashtable */
    hash_add(netdev_htable, &nddata->hnode, (int)nlpid);
    netdev_minors_used[MINOR(nddata->cdev->dev)] = nddata->nlpid;

    return 0; /* success */
undo_cdev:
    cdev_del(nddata->cdev);
free_nddata:
    ndmgm_free_data(nddata);
    return err;
}

int ndmgm_find_destroy(int nlpid)
{
    struct netdev_data *nddata = NULL;
    
    nddata = ndmgm_find(nlpid);

    if (down_read_trylock(&netdev_htable_sem)) {
        netdev_minors_used[MINOR(nddata->cdev->dev)] = 0;
        hash_del(&nddata->hnode);
        ndmgm_put(nddata);
        netdev_devices[MINOR(nddata->cdev->dev)] = NULL;
        ndmgm_put(nddata);

        up_read(&netdev_htable_sem);
    }

    return ndmgm_destroy(nddata);
}

int ndmgm_destroy(struct netdev_data *nddata)
{
    debug("destroying device %s", nddata->devname);
    if (nddata) {
        if (down_write_trylock(&nddata->sem)) {
            nddata->active = false;

            /* make sure all pending operations are completed */
            if (ndmgm_free_queue(nddata)) {
                printk(KERN_ERR "ndmgm_destroy: failed to free queue\n");
                return 1; /* failure */
            }
            /* should never happen but better test for it */
            if (ndmgm_refs(nddata) > 1) {
                printk(KERN_ERR "ndmgm_destroy: more than one ref left\n");
                return 1; /* failure */
            }

            device_destroy(netdev_class, nddata->cdev->dev);
            cdev_del(nddata->cdev);

            up_write(&nddata->sem); /* has to be unlocked before kfree */

            ndmgm_free_data(nddata); /* finally free netdev_data */

            netdev_count--;
            return 0; /* success */
        }
    }
    printk(KERN_ERR "ndmgm_destroy: failed to destroy netdev_data\n");
    return 1; /* failure */
}

int ndmgm_end(void)
{
    int i = 0;
    struct netdev_data *nddata = NULL;
    struct hlist_node *tmp;
    debug("cleaning devices");

    if (down_write_trylock(&netdev_htable_sem)) {
        debug("locked");
        /* needs to be _safe so we can delete elements inside the loop */
        hash_for_each_safe(netdev_htable, i, tmp, nddata, hnode) {
            debug("deleting dev pid = %d",
                                nddata->nlpid);
            netdev_minors_used[MINOR(nddata->cdev->dev)] = 0;
            hash_del(&nddata->hnode); /* delete the element from table */
            ndmgm_put(nddata);
            netdev_devices[MINOR(nddata->cdev->dev)] = NULL;
            ndmgm_put(nddata);

            if (ndmgm_destroy(nddata)) {
                printk(KERN_ERR "netdev_end: failed to destroy nddata\n");
            }
        }
        up_write(&netdev_htable_sem);
        return 0; /* success */
    }
    return 1; /* failure */
}

void ndmgm_prepare(void)
{
    /* create and array for all drivices which will be indexed with
     * minor numbers of those devices */
    netdev_minors_used = (int*)kcalloc(NETDEV_MAX_DEVICES,
                                            sizeof(*netdev_minors_used),
                                            GFP_KERNEL);
    /* create the hashtable which will store data about created devices
     * and for easy access through pid */
    hash_init(netdev_htable);
    netdev_count = 0;
}

struct netdev_data* ndmgm_find(int nlpid)
{
    struct netdev_data *nddata = NULL;

    if (down_read_trylock(&netdev_htable_sem)) {
        hash_for_each_possible(netdev_htable, nddata, hnode, (int)nlpid) {
            if ( nddata->nlpid == nlpid ) {
                up_read(&netdev_htable_sem);
                ndmgm_get(nddata);
                return nddata;
            }
        }
        up_read(&netdev_htable_sem);
    }

    return NULL;
}

void ndmgm_get(struct netdev_data *nddata)
{
    //debug("called from: %pS", __builtin_return_address(0));
    atomic_inc(&nddata->users);
}

void ndmgm_put(struct netdev_data *nddata)
{
    //debug("called from: %pS", __builtin_return_address(0));
    atomic_dec(&nddata->users);
}

int ndmgm_refs(struct netdev_data *nddata)
{
    return atomic_read(&nddata->users);
}

#include <linux/slab.h>     /* kmalloc, kzalloc, kfree and so on */
#include <linux/device.h> 

#include "netdevmgm.h"
#include "fo.h"
#include "dbg.h"

static DEFINE_HASHTABLE(netdev_htable, NETDEV_HASHTABLE_SIZE);
static struct rw_semaphore netdev_htable_sem;
unsigned int netdev_count;
int *netdev_minors_used;

struct netdev_data * ndmgm_alloc_data(
    int nlpid,
    char *name)
{
    int err = 0;
    struct netdev_data *nddata; /* TODO needs to be in a list */

    /* check if we have space for another device */
    /* TODO needs a reuse of device numbers from netdev_minors_used array */
    if ( netdev_count + 1 > NETDEV_MAX_DEVICES ) {
        printk(KERN_ERR "ndmgm_alloc_data: max devices reached = %d\n",
                        NETDEV_MAX_DEVICES);
        return 0;
    }

    /* GFP_KERNEL means this function can be blocked,
     * so it can't be part of an atomic operation.
     * For that GFP_ATOMIC would have to be used. */
    nddata = kzalloc(sizeof(*nddata), GFP_KERNEL);
    if (!nddata) {
        printk(KERN_ERR "ndmgm_alloc_data: failed to allocate nddata\n");
        return NULL;
    }

    nddata->devname = kzalloc(strlen(name), GFP_KERNEL);
    if (!nddata->devname) {
        printk(KERN_ERR "ndmgm_alloc_data: failed to allocate nddata\n");
        goto free_devname;
    }

    nddata->cdev = kzalloc(sizeof(*nddata->cdev), GFP_KERNEL);
    if (!nddata->cdev) {
        printk(KERN_ERR "ndmgm_alloc_data: failed to allocate nddata->cdev\n");
        goto free_nddata;
    }

    if ((err = kfifo_alloc(&nddata->fo_queue, NETDEV_FO_QUEUE_SIZE, GFP_KERNEL))) {
        printk(KERN_ERR "ndmgm_alloc_data: failed to allocate fo_queue\n");
        goto free_cdev;
    }

    nddata->queue_pool = kmem_cache_create(NETDEV_REQ_POOL_NAME,
                        sizeof(struct fo_req),
                        0, 0, NULL); /* no alignment, flags or constructor */
    if (!nddata->queue_pool) {
        printk(KERN_ERR "ndmgm_alloc_data: failed to allocate queue_pool\n");
        goto free_fo_queue;
    }

    sprintf(nddata->devname, "/dev/%s%d", name, netdev_count);
    init_rwsem(&nddata->sem);
    atomic_set(&nddata->curseq, 0);
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

void ndmgm_free_data(
    struct netdev_data *nddata)
{
    kmem_cache_destroy(nddata->queue_pool);
    kfifo_free(&nddata->fo_queue);
    kfree(nddata->cdev);
    kfree(nddata->devname);
    kfree(nddata);
}

int ndmgm_free_queue(
    struct netdev_data *nddata)
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
    }

    return 0; /* success */
}

/* find the filo operation request with the correct sequence number */
struct fo_req * ndmgm_foreq_find(
    struct netdev_data *nddata,
    int seq)
{
    /* will hold the first element as a stopper */
    struct fo_req *req = NULL;
    struct fo_req *tmp = NULL;
    int size = 0;

    if (down_write_trylock(&nddata->sem)) {
        while (!kfifo_is_empty(&nddata->fo_queue)) {
            size = kfifo_out(&nddata->fo_queue, &tmp, sizeof(tmp));
            if (size < sizeof(tmp) || IS_ERR(tmp)) {
                printk(KERN_ERR "ndmgm_foreq_find: failed to get queue element\n");
                tmp = NULL;
                break;
            }
            if (req == NULL) {
                req = tmp; /* first element */
            }
            if (tmp->seq == seq) {
                break;
            }
            /* put the wrong element back into the queue */
            kfifo_in(&nddata->fo_queue, &tmp, sizeof(tmp));
            if (req == tmp) {
                tmp = NULL;
                break;
            }
        }
        up_write(&nddata->sem);
    }
    return tmp;
}

int ndmgm_foreq_add(
    struct netdev_data *nddata,
    struct fo_req *req)
{
    if (down_write_trylock(&nddata->sem)) {
        kfifo_in(&nddata->fo_queue, &req, sizeof(req));
        up_write(&nddata->sem);
        return 0; /* success */
    }
    return 1; /* failure */
}

/* this function safely increases the current sequence number */
int ndmgm_incseq(struct netdev_data *nddata)
{
    atomic_inc(&nddata->curseq);
    return atomic_read(&nddata->users);
}

/*  TODO all this shit will HAVE to use semafors if it has to work */
int ndmgm_create_dummy(
    int nlpid,
    char *name)
{
    int err = 0;
    struct netdev_data *nddata = NULL;
    debug("creating dummy device: /dev/%s%d", name, netdev_count);

    if ( (nddata = ndmgm_alloc_data(nlpid, name)) == NULL ) {
        printk(KERN_ERR "ndmgm_create_dummy: failed to create netdev_data\n");
        return 1;
    }

    cdev_init(nddata->cdev, &netdev_fops);
    nddata->cdev->owner = THIS_MODULE;
    nddata->cdev->dev = MKDEV(MAJOR(netdev_devno), netdev_count);
    nddata->dummy = true; /* should be true for a dummy device */

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

int ndmgm_create_server(
    int nlpid,
    char *name)
{
    struct netdev_data *nddata = NULL;
    debug("creating server for: %s", name);

    nddata = kzalloc(sizeof(*nddata), GFP_KERNEL);
    if (!nddata) {
        printk(KERN_ERR "ndmgm_create_server: failed to allocate nddata\n");
        return 1;
    }

    nddata->devname = kzalloc(strlen(name), GFP_KERNEL);
    if (!nddata->devname) {
        printk(KERN_ERR "ndmgm_create_server: failed to allocate nddata\n");
        goto free_nddata;
    }

    memcpy(nddata->devname, name, strlen(name)+1);
    init_rwsem(&nddata->sem);
    atomic_set(&nddata->curseq, 0);
    nddata->nlpid = nlpid;
    nddata->active = true;
    nddata->dummy = false; /* should be false for a server device */

    /* add the device to hashtable with all devices since it's ready */
    ndmgm_get(nddata); /* increase count for hashtable */
    hash_add(netdev_htable, &nddata->hnode, (int)nlpid);

    /* TODO find cdev and device of the dev we are serving */

    return 0; /* success */
free_nddata:
    ndmgm_free_data(nddata);
    return 1;
}

int ndmgm_find_destroy(
    int nlpid)
{
    struct netdev_data *nddata = NULL;
    
    nddata = ndmgm_find(nlpid);
    if (!nddata) {
        printk(KERN_ERR "ndmgm_find_destroy: no such device\n");
        return 1; /* failure */
    }

    if (down_read_trylock(&netdev_htable_sem)) {
        hash_del(&nddata->hnode);
        ndmgm_put(nddata);

        up_read(&netdev_htable_sem);
    }

    return ndmgm_destroy(nddata);
}

int ndmgm_destroy(
    struct netdev_data *nddata)
{
    int rvalue;
    debug("destroying device %s", nddata->devname);
    if (nddata) {
        printk(KERN_ERR "ndmgm_destroy: nddata is NULL\n");
        return 1; /* failure */
    }
    
    if (nddata->dummy == true) {
        rvalue = ndmgm_destroy_dummy(nddata);
    } else if (nddata->dummy == false) {
        rvalue = ndmgm_destroy_server(nddata);
    }

    return rvalue;
}

int ndmgm_destroy_dummy(
    struct netdev_data *nddata)
{
    if (down_write_trylock(&nddata->sem)) {
        nddata->active = false;
        netdev_minors_used[MINOR(nddata->cdev->dev)] = 0;

        /* make sure all pending operations are completed */
        if (ndmgm_free_queue(nddata)) {
            printk(KERN_ERR "ndmgm_destroy_dummy: failed to free queue\n");
            return 1; /* failure */
        }
        /* should never happen but better test for it */
        if (ndmgm_refs(nddata) > 1) {
            printk(KERN_ERR "ndmgm_destroy_dummy: more than one ref left: %d\n", ndmgm_refs(nddata));
            return 1; /* failure */
        }

        device_destroy(netdev_class, nddata->cdev->dev);
        cdev_del(nddata->cdev);

        up_write(&nddata->sem); /* has to be unlocked before kfree */

        ndmgm_free_data(nddata); /* finally free netdev_data */

        netdev_count--;
        return 0; /* success */
    }
    printk(KERN_ERR "ndmgm_destroy_dummy: failed to destroy netdev_data\n");
    return 1; /* failure */
}

int ndmgm_destroy_server(
    struct netdev_data *nddata)
{
    if (down_write_trylock(&nddata->sem)) {
        nddata->active = false;

        if (ndmgm_refs(nddata) > 1) {
            printk(KERN_ERR "ndmgm_destroy_server: more than one ref left: %d\n", ndmgm_refs(nddata));
            return 1; /* failure */
        }

        up_write(&nddata->sem); /* has to be unlocked before kfree */

        kfree(nddata);

        return 0; /* success */
    }
    printk(KERN_ERR "ndmgm_destroy_server: failed to destroy netdev_data\n");
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
            hash_del(&nddata->hnode); /* delete the element from table */
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

/* returns netdev_data based on pid, you have to make sure to
 * increment the reference counter */
struct netdev_data* ndmgm_find(int nlpid)
{
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

void ndmgm_get(
    struct netdev_data *nddata)
{
    //debug("called from: %pS", __builtin_return_address(0));
    atomic_inc(&nddata->users);
}

void ndmgm_put(
    struct netdev_data *nddata)
{
    //debug("called from: %pS", __builtin_return_address(0));
    atomic_dec(&nddata->users);
}

int ndmgm_refs(
    struct netdev_data *nddata)
{
    return atomic_read(&nddata->users);
}

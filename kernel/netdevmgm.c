#include <linux/slab.h>     /* kmalloc, kzalloc, kfree and so on */
#include <linux/device.h>

#include "netdevmgm.h"
#include "fo.h"
#include "dbg.h"

static DEFINE_HASHTABLE(netdev_htable, NETDEV_HTABLE_DEV_SIZE);
static struct rw_semaphore  netdev_htable_sem;
static struct rw_semaphore  netdev_minor_sem;
static int netdev_max_minor;
static int *netdev_minors_used; /* array with pids to all the devices */

struct netdev_data * ndmgm_alloc_data(
    int nlpid,
    char *name,
    int minor)
{
    struct netdev_data *nddata;
    char pool_name[30];
    sprintf(pool_name, "%s:%s%d", NETDEV_REQ_POOL_NAME, name, minor);

    nddata = kzalloc(sizeof(*nddata), GFP_KERNEL);
    if (!nddata) {
        printk(KERN_ERR "ndmgm_alloc_data: failed to allocate nddata\n");
        return NULL;
    }

    nddata->devname = kzalloc(strlen(name)+1, GFP_KERNEL);
    if (!nddata->devname) {
        printk(KERN_ERR "ndmgm_alloc_data: failed to allocate nddata\n");
        goto free_devname;
    }

    nddata->cdev = kzalloc(sizeof(*nddata->cdev), GFP_KERNEL);
    if (!nddata->cdev) {
        printk(KERN_ERR "ndmgm_alloc_data: failed to allocate nddata->cdev\n");
        goto free_nddata;
    }

    nddata->queue_pool = kmem_cache_create(pool_name,
                                            sizeof(struct fo_req),
    /* no alignment, flags or constructor */ 0, 0, NULL);

    if (!nddata->queue_pool) {
        printk(KERN_ERR "ndmgm_alloc_data: failed to allocate queue_pool\n");
        goto free_cdev;
    }

    sprintf(nddata->devname, "/dev/%s", name);
    hash_init(nddata->foacc_htable);
    init_rwsem(&nddata->sem);
    spin_lock_init(&nddata->nllock);
    atomic_set(&nddata->curseq, 0);
    nddata->nlpid = nlpid;
    nddata->active = true;

    return nddata;
free_cdev:
    kfree(nddata->cdev);
free_devname:
    kfree(nddata->devname);
free_nddata:
    kfree(nddata);
    return NULL;
}

int ndmgm_create_dummy(
    int nlpid,
    char *name)
{
    int err = 0;
    int minor = -1;
    struct netdev_data *nddata = NULL;

    minor = ndmgm_get_minor(nlpid);
    if (minor == -1) {
        printk(KERN_ERR "ndmgm_create_dummy: could not get minor\n");
        return 1;
    }
    debug("creating dummy device: /dev/%s", name);

    if ( (nddata = ndmgm_alloc_data(nlpid, name, minor)) == NULL ) {
        printk(KERN_ERR "ndmgm_create_dummy: failed to create netdev_data\n");
        return 1;
    }

    cdev_init(nddata->cdev, &netdev_fops);
    nddata->cdev->owner = THIS_MODULE;
    nddata->cdev->dev = MKDEV(MAJOR(netdev_devno), minor);
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
                            NULL,   /* no aprent device           */
                            nddata->cdev->dev, /* major and minor */
                            nddata, /* device data for callback   */
                            "%s", /* defines name of the device */
                            name);

    if (IS_ERR(nddata->device)) {
       err = PTR_ERR(nddata->device);
       printk(KERN_WARNING "[target] Error %d while trying to name %s\n",
                            err, name);
       goto undo_cdev;
    }

    /* add the device to hashtable with all devices since it's ready */
    ndmgm_get(nddata); /* increase count for hashtable */
    hash_add(netdev_htable, &nddata->hnode, (int)nlpid);

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
    char pool_name[30];
    sprintf(pool_name, "%s:%s", NETDEV_REQ_POOL_NAME, name);

    debug("creating server for: %s", name);

    nddata = kzalloc(sizeof(*nddata), GFP_KERNEL);
    if (!nddata) {
        printk(KERN_ERR "ndmgm_create_server: failed to allocate nddata\n");
        return 1;
    }

    nddata->devname = kzalloc(strlen(name)+1, GFP_KERNEL);
    if (!nddata->devname) {
        printk(KERN_ERR "ndmgm_create_server: failed to allocate nddata\n");
        goto free_nddata;
    }

    nddata->queue_pool = kmem_cache_create(pool_name,
                                            sizeof(struct fo_req),
    /* no alignment, flags or constructor */ 0, 0, NULL);

    if (!nddata->queue_pool) {
        printk(KERN_ERR "ndmgm_alloc_data: failed to allocate queue_pool\n");
        goto free_devname;
    }


    memcpy(nddata->devname, name, strlen(name)+1);
    init_rwsem(&nddata->sem);
    spin_lock_init(&nddata->nllock);
    atomic_set(&nddata->curseq, 0);
    nddata->nlpid = nlpid;
    nddata->active = true;
    nddata->dummy = false; /* should be false for a server device */

    /* add the device to hashtable with all devices since it's ready */
    ndmgm_get(nddata); /* increase count for hashtable */
    hash_add(netdev_htable, &nddata->hnode, (int)nlpid);

    /* TODO find cdev and device of the dev we are serving */

    return 0; /* success */
free_devname:
    kfree(nddata->devname);
free_nddata:
    kfree(nddata);
    return 1;
}

void ndmgm_free_data(
    struct netdev_data *nddata)
{
    kmem_cache_destroy(nddata->queue_pool);
    kfree(nddata->cdev);
    kfree(nddata->devname);
    kfree(nddata);
}

/* returns netdev_data based on pid, you have to make sure to
 * increment the reference counter */
struct netdev_data* ndmgm_find(int nlpid)
{
    struct netdev_data *nddata = NULL;
    if (nlpid == -1) {
        printk("ndmgm_find: invalid pid\n");
        return NULL;
    }

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

struct fo_access * ndmgm_find_acc(
    struct netdev_data *nddata,
    int access_id)
{
    struct fo_access *acc = NULL;

    if (down_read_trylock(&nddata->sem)) {
        hash_for_each_possible(nddata->foacc_htable,
                                acc,
                                hnode,
                                access_id) {
            if ( acc->access_id == access_id ) {
                up_read(&nddata->sem);
                return acc;
            }
        }
        up_read(&nddata->sem);
    }

    return NULL;
}

void ndmgm_put_minor(
    int minor)
{
    int i = 0;
    if (down_write_trylock(&netdev_minor_sem)) {
        netdev_minors_used[minor] = -1;
        if (minor == netdev_max_minor) {
            for (i = minor; i > 0; i--) {
                if (netdev_minors_used[i] != -1) {
                    netdev_max_minor = i;
                }
            }
        }
        up_write(&netdev_minor_sem);
    }
}

int ndmgm_get_minor(
    int pid)
{
    int minor = -1;
    int i = 0;
    if (down_write_trylock(&netdev_minor_sem)) {
        if (netdev_max_minor == NETDEV_MAX_DEVICES) {
            printk("ndmgm_get_minor: device limit reached: %d\n",
                    NETDEV_MAX_DEVICES);
            up_read(&netdev_minor_sem);
            return -1;
        }
        for (i = 0; i <= netdev_max_minor+1; i++) {
            if (netdev_minors_used[i] == -1) {
                netdev_minors_used[i] = pid;
                minor = i;
                break;
            }
        }
        if (minor >= 0) {
            netdev_max_minor = max(netdev_max_minor, minor);
        }
        up_write(&netdev_minor_sem);
    }
    return minor;
}

int ndmgm_find_pid(
    dev_t dev)
{
    int pid = -1;
    if (down_read_trylock(&netdev_minor_sem)) {
        pid = netdev_minors_used[MINOR(dev)];
        up_read(&netdev_minor_sem);
    }
    return pid;
}

/* this function safely increases the current sequence number */
int ndmgm_incseq(
    struct netdev_data *nddata)
{
    int rvalue;
    atomic_inc(&nddata->curseq);
    rvalue = atomic_read(&nddata->curseq);
    return rvalue;
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
    if (!nddata) {
        printk(KERN_ERR "ndmgm_destroy: nddata is NULL\n");
        return 1; /* failure */
    }
    debug("destroying device %s", nddata->devname);

    if (nddata->dummy == true) {
        rvalue = ndmgm_destroy_dummy(nddata);
    } else if (nddata->dummy == false) {
        rvalue = ndmgm_destroy_server(nddata);
    }

    return rvalue;
}

int ndmgm_destroy_allacc(
    struct netdev_data *nddata)
{
    struct fo_access *acc = NULL;
    struct hlist_node *tmp;
    int i = 0;

    if (down_write_trylock(&netdev_htable_sem)) {
        hash_for_each_safe(nddata->foacc_htable, i, tmp, acc, hnode) {
            hash_del(&nddata->hnode); /* delete the element from table */

            /* make sure all pending operations are completed */
            if (fo_acc_free_queue(acc)) {
                printk(KERN_ERR "ndmgm_destroy_allacc: failed to free queue\n");
                return 1; /* failure */
            }

            fo_acc_destroy(acc);
        }
        up_write(&netdev_htable_sem);
        return 0; /* success */
    }
    return 1; /* failure */
}

int ndmgm_destroy_dummy(
    struct netdev_data *nddata)
{
    if (down_write_trylock(&nddata->sem)) {
        nddata->active = false;

        if (ndmgm_destroy_allacc(nddata)) {
            printk(KERN_ERR
                    "ndmgm_destroy_dummy: failed to stop all acc\n");
            up_write(&nddata->sem);
            return 1; /* failure */
        }

        /* should never happen but better test for it */
        if (ndmgm_refs(nddata) > 1) {
            printk(KERN_ERR
                    "ndmgm_destroy_dummy: more than one ref left: %d\n",
                    ndmgm_refs(nddata));
            up_write(&nddata->sem);
            return 1; /* failure */
        }

        device_destroy(netdev_class, nddata->cdev->dev);
        cdev_del(nddata->cdev);

        up_write(&nddata->sem); /* has to be unlocked before kfree */

        ndmgm_put_minor(MINOR(nddata->cdev->dev));
        ndmgm_free_data(nddata); /* finally free netdev_data */
        return 0; /* success */
    }
    debug("failed to destroy netdev_data");
    return 1; /* failure */
}

int ndmgm_destroy_server(
    struct netdev_data *nddata)
{
    if (down_write_trylock(&nddata->sem)) {
        nddata->active = false;

        if (ndmgm_refs(nddata) > 1) {
            printk(KERN_ERR
                    "ndmgm_destroy_server: more than one ref left: %d\n",
                    ndmgm_refs(nddata));
            up_write(&nddata->sem); /* has to be unlocked before kfree */
            return 1; /* failure */
        }

        up_write(&nddata->sem); /* has to be unlocked before kfree */

        kmem_cache_destroy(nddata->queue_pool);
        kfree(nddata->devname);
        kfree(nddata);

        return 0; /* success */
    }
    debug("failed to destroy netdev_data");
    return 1; /* failure */
}

int ndmgm_end(void)
{
    int i = 0;
    struct netdev_data *nddata = NULL;
    struct hlist_node *tmp;
    debug("cleaning devices");

    if (down_write_trylock(&netdev_htable_sem)) {
        hash_for_each_safe(netdev_htable, i, tmp, nddata, hnode) {
            debug("deleting dev pid = %d", nddata->nlpid);
            hash_del(&nddata->hnode); /* delete the element from table */
            ndmgm_put(nddata);

            if (ndmgm_destroy(nddata)) {
                printk(KERN_ERR "netdev_end: failed to destroy nddata\n");
            }
        }
        kfree(netdev_minors_used);
        up_write(&netdev_htable_sem);
        return 0; /* success */
    }
    debug("failed to unlock hashtable");
    return 1; /* failure */
}

void ndmgm_prepare(void)
{
    int i;
    /* create and array for all drivices which will be indexed with
     * minor numbers of those devices */
    netdev_minors_used = kcalloc(NETDEV_MAX_DEVICES,
                                sizeof(*netdev_minors_used),
                                GFP_KERNEL);
    for (i = 0; i < NETDEV_MAX_DEVICES; i++) {
        netdev_minors_used[i] = -1; /* -1 means free minor number */
    }
    netdev_max_minor = 0;

    /* create the hashtable which will store data about created devices
     * and for easy access through pid */
    hash_init(netdev_htable);
}

void ndmgm_get(
    struct netdev_data *nddata)
{
    //debug("dev: %15s, from: %pS", nddata->devname, __builtin_return_address(0));
    atomic_inc(&nddata->users);
}

void ndmgm_put(
    struct netdev_data *nddata)
{
    //debug("dev: %15s, from: %pS", nddata->devname, __builtin_return_address(0));
    atomic_dec(&nddata->users);
}

int ndmgm_refs(
    struct netdev_data *nddata)
{
    return atomic_read(&nddata->users);
}

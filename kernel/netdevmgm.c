#include <linux/slab.h>     /* kmalloc, kzalloc, kfree and so on */
#include <linux/device.h> 

#include "netdevmgm.h"
#include "fo.h"

static DEFINE_HASHTABLE(netdev_htable, NETDEV_HASHTABLE_SIZE);
static struct rw_semaphore netdev_htable_sem;
unsigned int netdev_count;

/*  TODO all this shit will HAVE to use semafors if it has to work */
int netdev_create(int nlpid, char *name) {
    int err;
    struct netdev_data *nddata; /* TODO needs to be in a list */

    /* check if we have space for another device */
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
    nddata->cdev = (struct cdev *) kzalloc(
                                sizeof(struct cdev), GFP_KERNEL);

    init_rwsem(&nddata->sem); /* lock the semaphor to avoid rece */
    nddata->nlpid = nlpid;
    nddata->devname = name;

    printk(KERN_DEBUG "netdev_create: creating device \\dev\\%s%d\n",
                        name,
                        netdev_count);

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
        goto fail;
    }

    nddata->device = device_create(netdev_class,
                    NULL,              /* no aprent device           */
                    nddata->cdev->dev, /* major and minor numbers    */
                    nddata,            /* private data of device     */
                    "%s%d",            /* defines name of the device */
                    name,
                    netdev_count);

    if (IS_ERR(nddata->device)) {
       err = PTR_ERR(nddata->device);
       printk(KERN_WARNING "[target] Error %d while trying to name %s%d\n",
                            err,
                            name,
                            netdev_count);
       goto fail;
    }

    printk(KERN_DEBUG "netdev: new device: %d, %d\n",
                        MAJOR(nddata->cdev->dev),
                        MINOR(nddata->cdev->dev));

    netdev_count++;
    /* add the device to hashtable with all devices since it's ready */
    hash_add(netdev_htable, &nddata->hnode, (int)nlpid);

    return 1;
fail:
    netdev_destroy(nlpid);
    return 0;
}

int netdev_destroy(int nlpid) {
    struct netdev_data *nddata = NULL;
    struct hlist_node *tmp;

    if (down_read_trylock(&netdev_htable_sem)) {
        hash_for_each_possible_safe(netdev_htable,
                                    nddata,
                                    tmp,
                                    hnode,
                                    (int)nlpid) {
            if (nddata->nlpid == nlpid) {
                hash_del(&nddata->hnode);
            }
        }
        up_read(&netdev_htable_sem);
    }

    if (nddata) {
        if (down_write_trylock(&netdev_htable_sem)) {
            cdev_del(nddata->cdev);

            device_destroy(netdev_class, nddata->cdev->dev);
            /* kfree can take null as argument, no test needed */
            kfree(nddata);
            
            up_write(&netdev_htable_sem);
            return 0; /* success */
        }
    }

    return 1;
}

void netdev_htable_init(void) {
    /* create the hashtable which will store data about created devices
     * and for easy access through pid */
    hash_init(netdev_htable);
    netdev_count = 0;
}

struct netdev_data* netdev_find(int nlpid) {
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

int netdev_end(void) {
    int i = 0;
    struct netdev_data *nddata = NULL;
    struct hlist_node *tmp;

    if (down_write_trylock(&netdev_htable_sem)) {
        /* needs to be _safe so we can delete elements inside the loop */
        hash_for_each_safe(netdev_htable, i, tmp, nddata, hnode) {
            printk(KERN_DEBUG "netdev_end: deleting netdev->nlpid = %d\n",
                    nddata->nlpid);
            hash_del(&nddata->hnode); /* delete the element from table */

            if (nddata) {
                device_destroy(netdev_class, nddata->cdev->dev);

                cdev_del(nddata->cdev);
            }
            /* kfree can take null as argument, no test needed */
            kfree(nddata->cdev);
            kfree(nddata);
        }
        up_write(&netdev_htable_sem);
        return 0; /* success */
    }
    return 1; /* failure */
}

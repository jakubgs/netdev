#include <linux/slab.h>     /* kmalloc, kzalloc, kfree and so on */
#include <linux/device.h> 

#include "netdevmgm.h"
#include "fo.h"

static DEFINE_HASHTABLE(netdev_htable, NETDEV_HASHTABLE_SIZE);

/*  TODO all this shit will HAVE to use semafors if it has to work */
int netdev_create(int nlpid, char *name) {
    int err;
    struct netdev_data *nddata; /* TODO needs to be in a list */

    /* GFP_KERNEL means this function can be blocked,
     * so it can't be part of an atomic operation.
     * For that GFP_ATOMIC would have to be used. */
    nddata = (struct netdev_data *) kzalloc(sizeof(struct netdev_data), GFP_KERNEL);
    nddata->cdev = (struct cdev *) kzalloc(sizeof(struct cdev), GFP_KERNEL);

    nddata->nlpid = nlpid;
    nddata->devname = name;

    /* add the device to hashtable with all devices */
    hash_add(netdev_htable, &nddata->hnode, (int)nlpid);


    printk(KERN_DEBUG "netdev_create: creating device \"%s\"\n", name);

    cdev_init(nddata->cdev, &netdev_fops);
    nddata->cdev->owner = THIS_MODULE;

    /* tell the kernel the cdev structure is ready,
     * if it is not do not call cdev_add */
    err = cdev_add(nddata->cdev, netdev_devno, 1);

    /* Unlikely but might fail */
    if (unlikely(err)) {
        printk(KERN_ERR "Error %d adding netdev\n", err);
        goto fail;
    }

    nddata->device = device_create(netdev_class, NULL,
                                netdev_devno, NULL,
                                "%s%d", /* TODO dev file name */
                                name,
                                MINOR(netdev_devno));

    if (IS_ERR(nddata->device)) {
       err = PTR_ERR(nddata->device);
       printk(KERN_WARNING "[target] Error %d while trying to name %s%d\n",
                            err,
                            name,
                            MINOR(netdev_devno));
       goto fail;
    }

    printk(KERN_DEBUG "netdev: new device: %d, %d\n",
                        MAJOR(netdev_devno),
                        MINOR(netdev_devno));

    return 1;
fail:
    netdev_destroy(nlpid, nddata);
    return 0;
}

int netdev_destroy(int nlpid, struct netdev_data *nddata) {
    if (nddata) {
        cdev_del(nddata->cdev);

        device_destroy(netdev_class, nddata->cdev->dev);
    }

    /* kfree can take null as argument, no test needed */
    kfree(nddata);

    return 1;
}

void netdev_htable_init(void) {
    /* create the hashtable which will store data about created devices
     * and for easy access through pid */
    hash_init(netdev_htable);
}

struct netdev_data* netdev_find(int nlpid) {
    struct netdev_data *nddata;

    hash_for_each_possible(netdev_htable, nddata, hnode, (int)nlpid) {
        if ( nddata->nlpid == nlpid ) {
            return nddata;
        }
    }

    return NULL;
}

void netdev_end(void) {
    int i = 0;
    struct netdev_data *nddata;
    struct hlist_node *tmp;

    hash_for_each_safe(netdev_htable, i, tmp, nddata, hnode) {
        printk(KERN_DEBUG "netdev_end: deleting netdev->nlpid = %d\n",
                nddata->nlpid);
        hash_del(&nddata->hnode);

        if (nddata) {
            device_destroy(netdev_class, nddata->cdev->dev);

            cdev_del(nddata->cdev);
        }
        /* kfree can take null as argument, no test needed */
        kfree(nddata->cdev);
        kfree(nddata);
    }
}

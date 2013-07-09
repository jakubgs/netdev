#include <linux/compiler.h>
#include <linux/module.h>   /* for MODULE_ macros */
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>     /* kmalloc, kzalloc, kfree and so on */

#include "fo.h"
#include "netlink.h"

MODULE_VERSION("0.0.1");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Jakub Sokołowski <panswiata_at_gmail_com>");
MODULE_DESCRIPTION("This is my first kernel driver. Please don't use it...");

#define NETDEV_NAME "netdev"

/* for now we use only one, but in future we will use one for every device */
struct netdev_data {
    struct cdev cdev;
    struct device *device;
    /* TODO
     * this struct will containt more data especially about host of the device
     * and device name, major, minor, and process pid of the server that will
     * be communicating through netlink
     */
};

static unsigned int netdev_major; /* user dynamic allocation */
static int dev_count = 1;
static struct netdev_data *nddata;
static struct class *netdev_class;
static dev_t devno;

struct file_operations netdev_fops = {
    .owner             = THIS_MODULE,
    .llseek            = netdev_fo_llseek,
    .read              = netdev_fo_read,
    .write             = netdev_fo_write,
    .aio_read          = netdev_fo_aio_read,
    .aio_write         = netdev_fo_aio_write,
    .readdir           = netdev_fo_readdir,
    .poll              = netdev_fo_poll,
    .unlocked_ioctl    = netdev_fo_unlocked_ioctl,
    .compat_ioctl      = netdev_fo_compat_ioctl,
    .mmap              = netdev_fo_mmap,
    .open              = netdev_fo_open,
    .flush             = netdev_fo_flush,
    .release           = netdev_fo_release,
    .fsync             = netdev_fo_fsync,
    .aio_fsync         = netdev_fo_aio_fsync,
    .fasync            = netdev_fo_fasync,
    .lock              = netdev_fo_lock,
    .sendpage          = netdev_fo_sendpage,
    .get_unmapped_area = netdev_fo_get_unmapped_area,
    .check_flags       = netdev_fo_check_flags,
    .flock             = netdev_fo_flock,
    .splice_write      = netdev_fo_splice_write,
    .splice_read       = netdev_fo_splice_read,
    .setlease          = netdev_fo_setlease,
    .fallocate         = netdev_fo_fallocate,
    .show_fdinfo       = netdev_fo_show_fdinfo
};

static void netdev_cleanup(void) {
    /* TODO
     * move to separate function that will be raised by netlink_recv as it gets
     * information from the server process that the connection is ending
     */
    if (nddata) {
        cdev_del(&nddata->cdev);

        device_destroy(netdev_class, devno);
    }

    if (netdev_class)
        class_destroy(netdev_class);

    /* netdev_cleanup is not called if alloc_chrdev_region has failed */
    unregister_chrdev_region(devno, dev_count);

    /* kfree can take null as argument, no test needed */
    kfree(nddata);

    printk(KERN_DEBUG "netdev: cleaned up after this device\n");

    return;
}

static int __init netdev_init(void) { /* Constructor */
    /* TODO
     * this whole section will have to be moved to a separate function that will
     * be used by netlink_recv to create devices as the server process sends
     * information about them
     */
    int err;
    netdev_major = 0;

    /* GFP_KERNEL means this function can be blocked,
     * so it can't be part of an atomic operation.
     * For that GFP_ATOMIC would have to be used. */
    nddata = (struct netdev_data *) kzalloc(sizeof(struct netdev_data), GFP_KERNEL);

    /* get a range of minor numbers (starting with 0) to work with */
    err = alloc_chrdev_region(&devno, netdev_major, dev_count, NETDEV_NAME);

    /* Fail gracefully if need be */
    if (err) {
        printk(KERN_ERR "netdev: error registering chrdev!\n");
        return err;
    }
    netdev_major = MAJOR(devno);

    netdev_class = class_create(THIS_MODULE, NETDEV_NAME);
    if (IS_ERR(netdev_class)) {
        err = PTR_ERR(netdev_class);
        goto fail;
    }

    cdev_init(&nddata->cdev, &netdev_fops);

    nddata->cdev.owner = THIS_MODULE;
    nddata->cdev.ops   = &netdev_fops;

    err = cdev_add(&nddata->cdev, devno, 1);

    /* Unlikely but might fail */
    if (unlikely(err)) {
        printk(KERN_ERR "Error %d adding netdev\n", err);
        goto fail;
    }

    nddata->device = device_create(netdev_class, NULL,
                                devno, NULL,
                                NETDEV_NAME "%d",
                                MINOR(devno));

    if (IS_ERR(nddata->device)) {
       err = PTR_ERR(nddata->device);
       printk(KERN_WARNING "[target] Error %d while trying to create %s%d\n",
                            err,
                            NETDEV_NAME,
                            MINOR(devno));
       goto fail;
    }

    printk(KERN_DEBUG "netdev: This is my new device: %d, %d\n",
                        MAJOR(devno),
                        MINOR(devno));

    // TEST NETLINK
    netlink_init();

    return 0; /* success */

fail:
    netdev_cleanup();
    return err;
}

static void __exit netdev_exit(void) { /* Destructor */
    /* first diable the netlink module then you can disable the device */
    netlink_exit();
    /* TODO
     * this should be really done by netlink code since it will disable devices
     * base on establised connections with netdev server process
     * */
    netdev_cleanup();
    return;
}


module_exit(netdev_exit);
module_init(netdev_init);

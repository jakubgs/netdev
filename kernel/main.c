#include <linux/compiler.h>
#include <linux/module.h>   /* for MODULE_ macros */
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>

#include "protocol.h"
#include "netlink.h"
#include "netdevmgm.h"
#include "fo.h"

MODULE_VERSION("0.0.1");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Jakub Sokołowski <panswiata_at_gmail_com>");
MODULE_DESCRIPTION("This is an experimental kernel driver. Please use at your own risk.");

static int dev_count = NETDEV_MAX_DEVICES; /* TODO driver option */
static int netdev_major = NETDEV_MAJOR; /* TODO driver option */
dev_t netdev_devno;
struct class *netdev_class;

static void netdev_cleanup(void)
{
    while (ndmgm_end()) {
        printk(KERN_ERR "netdev_cleanup: failed to delete devices");
    }

    if (netdev_class)
        class_destroy(netdev_class);

    /* netdev_cleanup is not called if alloc_chrdev_region has failed */
    /* free the major and minor numbers */
    unregister_chrdev_region(netdev_devno, dev_count);

    printk(KERN_DEBUG "netdev: driver cleanup finished\n");

    return;
}

static int __init netdev_init(void)
{
    int err;
    int first_minor = 0;
    /* get a range of minor numbers (starting with 0) to work with
     * we are using alloc rather than register to get dynamic major */
    err = alloc_chrdev_region(&netdev_devno, first_minor, dev_count, NETDEV_NAME);

    /* fail gracefully if need be */
    if (err) {
        printk(KERN_ERR "netdev: error registering chrdev!\n");
        return err;
    }
    netdev_major = MAJOR(netdev_devno);

    netdev_class = class_create(THIS_MODULE, NETDEV_NAME);
    if (IS_ERR(netdev_class)) {
        err = PTR_ERR(netdev_class);
        goto fail;
    }
    
    /* create array and hashtable for devices */
    ndmgm_prepare();

    /* setup netlink */
    err = netlink_init();
    if (err) {
        printk(KERN_ERR "netdev: netlink setup failed!\n");
        goto fail;
    }

    return 0; /* success */

fail:
    netdev_cleanup();
    return err;
}

static void __exit netdev_exit(void)
{
    netlink_exit();
    netdev_cleanup();
    return;
}

module_exit(netdev_exit);
module_init(netdev_init);

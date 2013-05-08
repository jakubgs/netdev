/* ofd.c – Our First Driver code */
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>

MODULE_VERSION("0.0.1");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Jakub Sokołowski <panswiata_at_gmail_com>");
MODULE_DESCRIPTION("This is my first kernel driver.");

static int __init netdev_init(void) /* Constructor */
{
    printk(KERN_INFO "netdev: Hello world!\n");
    return 0;
}

static void __exit netdev_exit(void) /* Destructor */
{
    printk(KERN_INFO "netdev: Goodbye, curel world\n");
}

module_exit(netdev_exit);
module_init(netdev_init);

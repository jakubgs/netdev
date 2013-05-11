/* ofd.c – Our First Driver code */
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>

MODULE_VERSION("0.0.1");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Jakub Sokołowski <panswiata_at_gmail_com>");
MODULE_DESCRIPTION("This is my first kernel driver.");

#define NETDEV_NAME "netdev"

struct netdev_dev {
    struct cdev cdev;
    struct device *device;
};

static unsigned int netdev_major = 0; // user dynamic allocation
static int dev_count = 1;
static struct netdev_dev *dev = NULL;
static struct class *netdev_class = NULL;
static dev_t devno;

loff_t  netdev_llseek (struct file *a, loff_t b, int c) { return -EIO; }
ssize_t netdev_read (struct file *a, char __user *b, size_t c, loff_t *d) { return -EIO; }
ssize_t netdev_write (struct file *a, const char __user *b, size_t c, loff_t *d) { return -EIO; }
ssize_t netdev_aio_read (struct kiocb *a, const struct iovec *b, unsigned long c, loff_t d) { return -EIO; }
ssize_t netdev_aio_write (struct kiocb *a, const struct iovec *b, unsigned long c, loff_t d) { return -EIO; }
int     netdev_readdir (struct file *a, void *b, filldir_t c) { return -EIO; }
unsigned int netdev_poll (struct file *a, struct poll_table_struct *b) { return -EIO; }
long    netdev_unlocked_ioctl (struct file *a, unsigned int b, unsigned long c) { return -EIO; }
long    netdev_compat_ioctl (struct file *a, unsigned int b, unsigned long c) { return -EIO; }
int     netdev_mmap (struct file *a, struct vm_area_struct *b) { return -EIO; }
int     netdev_open (struct inode *a, struct file *b) { return -EIO; }
int     netdev_flush (struct file *a, fl_owner_t id) { return -EIO; }
int     netdev_release (struct inode *a, struct file *b) { return -EIO; }
int     netdev_fsync (struct file *a, loff_t b, loff_t c, int d) { return -EIO; }
int     netdev_aio_fsync (struct kiocb *a, int b) { return -EIO; }
int     netdev_fasync (int a, struct file *b, int c) { return -EIO; }
int     netdev_lock (struct file *a, int b, struct file_lock *c) { return -EIO; }
ssize_t netdev_sendpage (struct file *a, struct page *b, int c, size_t d, loff_t *e, int f) { return -EIO; }
unsigned long netdev_get_unmapped_area(struct file *a, unsigned long b, unsigned long c, unsigned long d, unsigned long e) { return -EIO; }
int     netdev_check_flags(int a) { return -EIO; }
int     netdev_flock (struct file *a, int b, struct file_lock *c) { return -EIO; }
ssize_t netdev_splice_write(struct pipe_inode_info *a, struct file *b, loff_t *c, size_t d, unsigned int e) { return -EIO; }
ssize_t netdev_splice_read(struct file *a, loff_t *b, struct pipe_inode_info *c, size_t d, unsigned int e) { return -EIO; }
int     netdev_setlease(struct file *a, long b, struct file_lock **c) { return -EIO; }
long    netdev_fallocate(struct file *a, int b, loff_t offset, loff_t len) { return -EIO; }
int     netdev_show_fdinfo(struct seq_file *a, struct file *b) { return -EIO; }

struct file_operations netdev_fops = {
    .owner          = THIS_MODULE,
    .llseek         = netdev_llseek,
    .read           = netdev_read,
    .write          = netdev_write,
    .aio_read       = netdev_aio_read,
    .aio_write      = netdev_aio_write,
    .readdir        = netdev_readdir,
    .poll           = netdev_poll,
    .unlocked_ioctl = netdev_unlocked_ioctl,
    .compat_ioctl   = netdev_compat_ioctl,
    .mmap           = netdev_mmap,
    .open           = netdev_open,
    .flush          = netdev_flush,
    .release        = netdev_release,
    .fsync          = netdev_fsync,
    .aio_fsync      = netdev_aio_fsync,
    .fasync         = netdev_fasync,
    .lock           = netdev_lock,
    .sendpage       = netdev_sendpage,
    .get_unmapped_area = netdev_get_unmapped_area,
    .check_flags    = netdev_check_flags,
    .flock          = netdev_flock,
    .splice_write   = netdev_splice_write,
    .splice_read    = netdev_splice_read,
    .setlease       = netdev_setlease,
    .fallocate      = netdev_fallocate,
    .show_fdinfo    = netdev_show_fdinfo
};

static void netdev_cleanup(void) {
    if (dev) {
        cdev_del(&dev->cdev);

        device_destroy(netdev_class, devno);

        kfree(dev);
    }

    if (netdev_class)
        class_destroy(netdev_class);

    /* netdev_cleanup is not called if alloc_chrdev_region has failed */
    unregister_chrdev_region(devno, dev_count);

    printk(KERN_INFO "netdev: Goodbye, curel world\n");

    return;
}

static int __init netdev_init(void) /* Constructor */
{
    int err;

    dev = (struct netdev_dev *) kzalloc(sizeof(struct netdev_dev), GFP_KERNEL);

    /* get a range of minor numbers (starting with 0) to work with */
    err = alloc_chrdev_region(&devno, netdev_major, dev_count, NETDEV_NAME);

    /* Fail gracefully if need be */
    if (err) {
        printk(KERN_ERR "netdev: error registering chrdev!");
        return err;
    }
    netdev_major = MAJOR(devno);

    netdev_class = class_create(THIS_MODULE, NETDEV_NAME);
    if (IS_ERR(netdev_class)) {
        err = PTR_ERR(netdev_class);
        goto fail;
    }

    cdev_init(&dev->cdev, &netdev_fops);

    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &netdev_fops;

    err = cdev_add(&dev->cdev, dev_count, 1);

    /* Unlikely but might fail */
    if (err) {
        printk(KERN_ERR "Error %d adding netdev", err);
        return err;
    }

    dev->device = device_create(netdev_class, NULL, devno, NULL, NETDEV_NAME "%d", MINOR(devno));
   
    if (IS_ERR(dev->device)) {
       err = PTR_ERR(dev->device);
       printk(KERN_WARNING "[target] Error %d while trying to create %s%d", err, NETDEV_NAME, MINOR(devno));
       goto fail;
    }

    printk(KERN_INFO "netdev: This is my new device: %d, %d\n", MAJOR(devno), MINOR(devno));
    return 0; /* success */
    
fail:
    netdev_cleanup();
    return err;
}

static void __exit netdev_exit(void) /* Destructor */
{
    netdev_cleanup();
    return;
}


module_exit(netdev_exit);
module_init(netdev_init);

#include <linux/compiler.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>

#include "fo.h"

MODULE_VERSION("0.0.1");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Jakub Sokołowski <panswiata_at_gmail_com>");
MODULE_DESCRIPTION("This is my first kernel driver. Please don't use it...");

#define NETDEV_NAME "netdev"

/* for now we use only one, but in future we will use one for every device */
struct netdev_dev {
    struct cdev cdev;
    struct device *device;
    /* TODO
     * this struct will containt more data
     * especially about host of the device
     * and device name, major, minor, stuff like that
     */
};

static unsigned int netdev_major; /* user dynamic allocation */
static int dev_count = 1;
static struct netdev_dev *dev;
static struct class *netdev_class;
static dev_t devno;

static void pk(const char * name) {
    printk(KERN_INFO "netdev: File Operation called - %s\n", name);
}

loff_t  netdev_fo_llseek (
                            struct file *filp,
                            loff_t offset,
                            int c)
{
    pk("llseek");
    return netdev_fo_llseek_send(filp, offset, c);
}
ssize_t netdev_fo_read (
                            struct file *filp,
                            char __user *data,
                            size_t c,
                            loff_t *offset)
{ 
    pk("read");
    return netdev_fo_read_send(filp, data, c, offset);
}
ssize_t netdev_fo_write (
                            struct file *filp,
                            const char __user *data,
                            size_t c,
                            loff_t *offset)
{
    pk("write");
    return netdev_fo_write_send(filp, data, c, offset);
}
ssize_t netdev_fo_aio_read (
                            struct kiocb *a,
                            const struct iovec *b,
                            unsigned long c,
                            loff_t offset)
{
    pk("aio_read");
    return netdev_fo_aio_read_send(a, b ,c, offset);
}
ssize_t netdev_fo_aio_write (
                            struct kiocb *a,
                            const struct iovec *b,
                            unsigned long c,
                            loff_t offset)
{
    pk("aio_write");
    return netdev_fo_aio_write_send(a, b, c, offset);
}
int     netdev_fo_readdir (
                            struct file *filp,
                            void *b,
                            filldir_t c)
{
    pk("readdir");
    return netdev_fo_readdir_send(filp, b, c);
}
unsigned int netdev_fo_poll (
                            struct file *filp,
                            struct poll_table_struct *b)
{
    pk("poll");
    return netdev_fo_poll_send(filp, b);
}
long    netdev_fo_unlocked_ioctl (
                            struct file *filp,
                            unsigned int b,
                            unsigned long c)
{
    pk("unlocked_ioctl");
    return netdev_fo_unlocked_ioctl_send(filp, b, c);
}
long    netdev_fo_compat_ioctl (
                            struct file *filp,
                            unsigned int b,
                            unsigned long c)
{
    pk("compat_ioctl");
    return netdev_fo_compat_ioctl_send(filp, b, c);
}
int     netdev_fo_mmap (
                            struct file *filp,
                            struct vm_area_struct *b)
{
    pk("mmap");
    return netdev_fo_mmap_send(filp, b);
}
int     netdev_fo_open (
                            struct inode *inode,
                            struct file *filp)
{
    pk("open");
    return netdev_fo_open_send(inode, filp);
}
int     netdev_fo_flush (
                            struct file *filp,
                            fl_owner_t id)
{
    pk("flush");
    return netdev_fo_flush_send(filp, id);
}
int     netdev_fo_release (
                            struct inode *a,
                            struct file *b)
{
    pk("release");
    return netdev_fo_release_send(a, b);
}
int     netdev_fo_fsync (
                            struct file *filp,
                            loff_t b,
                            loff_t c,
                            int d)
{
    pk("fsync");
    return netdev_fo_fsync_send(filp, b, c, d);
}
int     netdev_fo_aio_fsync (
                            struct kiocb *a,
                            int b)
{
    pk("aio_fsync");
    return netdev_fo_aio_fsync_send(a, b);
}
int     netdev_fo_fasync (
                            int a,
                            struct file *b,
                            int c)
{
    pk("fasync");
    return netdev_fo_fasync_send(a, b, c);
}
int     netdev_fo_lock (
                            struct file *filp,
                            int b,
                            struct file_lock *c)
{
    pk("lock");
    return netdev_fo_lock_send(filp, b, c);
}
ssize_t netdev_fo_sendpage (
                            struct file *filp,
                            struct page *b,
                            int c,
                            size_t d,
                            loff_t *offset,
                            int f)
{
    pk("sendpage");
    return netdev_fo_sendpage_send(filp, b, c, d, offset, f);
}
unsigned long netdev_fo_get_unmapped_area(
                            struct file *filp,
                            unsigned long b,
                            unsigned long c,
                            unsigned long d,
                            unsigned long e)
{
    pk("get_unmapped_are");
    return netdev_fo_get_unmapped_area_send(filp, b, c, d, e);
}
int     netdev_fo_check_flags(
                            int a)
{
    pk("check_flag");
    return netdev_fo_check_flags_send(a);
}
int     netdev_fo_flock (
                            struct file *filp,
                            int b,
                            struct file_lock *c)
{
    pk("flock");
    return netdev_fo_flock_send(filp, b, c);
}
ssize_t netdev_fo_splice_write(
                            struct pipe_inode_info *a,
                            struct file *filp,
                            loff_t *offset,
                            size_t d,
                            unsigned int e)
{
    pk("splice_writ");
    return netdev_fo_splice_write_send(a, filp, offset, d, e);
}
ssize_t netdev_fo_splice_read(
                            struct file *filp,
                            loff_t *offset,
                            struct pipe_inode_info *c,
                            size_t d,
                            unsigned int e)
{
    pk("splice_rea");
    return netdev_fo_splice_read_send(filp, offset, c, d, e);
}
int     netdev_fo_setlease(
                            struct file *filp,
                            long b,
                            struct file_lock **c)
{
    pk("setleas");
    return netdev_fo_setlease_send(filp, b, c);
}
long    netdev_fo_fallocate(
                            struct file *filp,
                            int b,
                            loff_t offset,
                            loff_t len)
{
    pk("fallocat");
    return netdev_fo_fallocate_send(filp, b, offset, len);
}
int netdev_fo_show_fdinfo(
                            struct seq_file *a,
                            struct file *filp)
{
    pk("show_fdinf");
    return netdev_fo_show_fdinfo_send(a, filp);
}

struct file_operations netdev_fops = {
    .owner              = THIS_MODULE,
    .llseek             = netdev_fo_llseek,
    .read               = netdev_fo_read,
    .write              = netdev_fo_write,
    .aio_read           = netdev_fo_aio_read,
    .aio_write          = netdev_fo_aio_write,
    .readdir            = netdev_fo_readdir,
    .poll               = netdev_fo_poll,
    .unlocked_ioctl     = netdev_fo_unlocked_ioctl,
    .compat_ioctl       = netdev_fo_compat_ioctl,
    .mmap               = netdev_fo_mmap,
    .open               = netdev_fo_open,
    .flush              = netdev_fo_flush,
    .release            = netdev_fo_release,
    .fsync              = netdev_fo_fsync,
    .aio_fsync          = netdev_fo_aio_fsync,
    .fasync             = netdev_fo_fasync,
    .lock               = netdev_fo_lock,
    .sendpage           = netdev_fo_sendpage,
    .get_unmapped_area  = netdev_fo_get_unmapped_area,
    .check_flags        = netdev_fo_check_flags,
    .flock              = netdev_fo_flock,
    .splice_write       = netdev_fo_splice_write,
    .splice_read        = netdev_fo_splice_read,
    .setlease           = netdev_fo_setlease,
    .fallocate          = netdev_fo_fallocate,
    .show_fdinfo        = netdev_fo_show_fdinfo
};

static void netdev_cleanup(void) {
    if (dev) {
        cdev_del(&dev->cdev);

        device_destroy(netdev_class, devno);
    }

    if (netdev_class)
        class_destroy(netdev_class);

    /* netdev_cleanup is not called if alloc_chrdev_region has failed */
    unregister_chrdev_region(devno, dev_count);

    /* kfree can take null as argument, no test needed */
    kfree(dev);

    printk(KERN_DEBUG "netdev: cleaned up after this device\n");

    return;
}

static int __init netdev_init(void) /* Constructor */
{
    int err;
    netdev_major = 0;

    dev = (struct netdev_dev *) kzalloc(sizeof(struct netdev_dev), GFP_KERNEL);

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

    cdev_init(&dev->cdev, &netdev_fops);

    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &netdev_fops;

    err = cdev_add(&dev->cdev, devno, 1);

    /* Unlikely but might fail */
    if (unlikely(err)) {
        printk(KERN_ERR "Error %d adding netdev\n", err);
        goto fail;
    }

    dev->device = device_create(netdev_class, NULL,
                                devno, NULL,
                                NETDEV_NAME "%d",
                                MINOR(devno));
   
    if (IS_ERR(dev->device)) {
       err = PTR_ERR(dev->device);
       printk(KERN_WARNING "[target] Error %d while trying to create %s%d\n",
                            err,
                            NETDEV_NAME,
                            MINOR(devno));
       goto fail;
    }

    printk(KERN_DEBUG "netdev: This is my new device: %d, %d\n",
                        MAJOR(devno), 
                        MINOR(devno));
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

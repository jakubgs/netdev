#include <linux/compiler.h>
#include <linux/module.h>   /* for MODULE_ macros */
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/sched.h>    /* access to current->comm and current->pid */
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>     /* kmalloc, kzalloc, kfree and so on */
#include <linux/netlink.h>  /* for netlink sockets */

#include "netdev_main.h"
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
     * this struct will containt more data
     * especially about host of the device
     * and device name, major, minor, stuff like that
     */
};

static unsigned int netdev_major; /* user dynamic allocation */
static int dev_count = 1;
static struct netdev_data *nddata;
static struct class *netdev_class;
static dev_t devno;

static void pk(const char * name) {
    printk(KERN_INFO "netdev: File Operation called by \"%s\", PID: %d - %s\n",
            current->comm,
            current->pid,
            name);
}

loff_t  netdev_fo_llseek (
                            struct file *filp,
                            loff_t offset,
                            int whence)
{
    pk(__FUNCTION__);
    return netdev_fo_llseek_send_req(filp, offset, whence);
}
ssize_t netdev_fo_read (
                            struct file *filp,
                            char __user *data,
                            size_t c,
                            loff_t *offset)
{ 
    pk(__FUNCTION__);
    return netdev_fo_read_send_req(filp, data, c, offset);
}
ssize_t netdev_fo_write (
                            struct file *filp,
                            const char __user *data,
                            size_t c,
                            loff_t *offset)
{
    pk(__FUNCTION__);
    return netdev_fo_write_send_req(filp, data, c, offset);
}
ssize_t netdev_fo_aio_read (
                            struct kiocb *a,
                            const struct iovec *b,
                            unsigned long c,
                            loff_t offset)
{
    pk(__FUNCTION__);
    return netdev_fo_aio_read_send_req(a, b ,c, offset);
}
ssize_t netdev_fo_aio_write (
                            struct kiocb *a,
                            const struct iovec *b,
                            unsigned long c,
                            loff_t offset)
{
    pk(__FUNCTION__);
    return netdev_fo_aio_write_send_req(a, b, c, offset);
}
int     netdev_fo_readdir (
                            struct file *filp,
                            void *b,
                            filldir_t c)
{
    pk(__FUNCTION__);
    return netdev_fo_readdir_send_req(filp, b, c);
}
unsigned int netdev_fo_poll (
                            struct file *filp,
                            struct poll_table_struct *wait)
{
    pk(__FUNCTION__);
    return netdev_fo_poll_send_req(filp, wait);
}
long    netdev_fo_unlocked_ioctl (
                            struct file *filp,
                            unsigned int b,
                            unsigned long c)
{
    pk(__FUNCTION__);
    return netdev_fo_unlocked_ioctl_send_req(filp, b, c);
}
long    netdev_fo_compat_ioctl (
                            struct file *filp,
                            unsigned int b,
                            unsigned long c)
{
    pk(__FUNCTION__);
    return netdev_fo_compat_ioctl_send_req(filp, b, c);
}
int     netdev_fo_mmap (
                            struct file *filp,
                            struct vm_area_struct *b)
{
    pk(__FUNCTION__);
    return netdev_fo_mmap_send_req(filp, b);
}
int     netdev_fo_open (
                            struct inode *inode,
                            struct file *filp)
{
    pk(__FUNCTION__);
    return netdev_fo_open_send_req(inode, filp);
}
int     netdev_fo_flush (
                            struct file *filp,
                            fl_owner_t id)
{
    pk(__FUNCTION__);
    return netdev_fo_flush_send_req(filp, id);
}
int     netdev_fo_release (
                            struct inode *a,
                            struct file *b)
{
    pk(__FUNCTION__);
    return netdev_fo_release_send_req(a, b);
}
int     netdev_fo_fsync (
                            struct file *filp,
                            loff_t b,
                            loff_t c,
                            int d)
{
    pk(__FUNCTION__);
    return netdev_fo_fsync_send_req(filp, b, c, d);
}
int     netdev_fo_aio_fsync (
                            struct kiocb *a,
                            int b)
{
    pk(__FUNCTION__);
    return netdev_fo_aio_fsync_send_req(a, b);
}
int     netdev_fo_fasync (
                            int a,
                            struct file *b,
                            int c)
{
    pk(__FUNCTION__);
    return netdev_fo_fasync_send_req(a, b, c);
}
int     netdev_fo_lock (
                            struct file *filp,
                            int b,
                            struct file_lock *c)
{
    pk(__FUNCTION__);
    return netdev_fo_lock_send_req(filp, b, c);
}
ssize_t netdev_fo_sendpage (
                            struct file *filp,
                            struct page *b,
                            int c,
                            size_t d,
                            loff_t *offset,
                            int f)
{
    pk(__FUNCTION__);
    return netdev_fo_sendpage_send_req(filp, b, c, d, offset, f);
}
unsigned long netdev_fo_get_unmapped_area(
                            struct file *filp,
                            unsigned long b,
                            unsigned long c,
                            unsigned long d,
                            unsigned long e)
{
    pk(__FUNCTION__);
    return netdev_fo_get_unmapped_area_send_req(filp, b, c, d, e);
}
int     netdev_fo_check_flags(
                            int a)
{
    pk(__FUNCTION__);
    return netdev_fo_check_flags_send_req(a);
}
int     netdev_fo_flock (
                            struct file *filp,
                            int b,
                            struct file_lock *c)
{
    pk(__FUNCTION__);
    return netdev_fo_flock_send_req(filp, b, c);
}
ssize_t netdev_fo_splice_write(
                            struct pipe_inode_info *a,
                            struct file *filp,
                            loff_t *offset,
                            size_t d,
                            unsigned int e)
{
    pk(__FUNCTION__);
    return netdev_fo_splice_write_send_req(a, filp, offset, d, e);
}
ssize_t netdev_fo_splice_read(
                            struct file *filp,
                            loff_t *offset,
                            struct pipe_inode_info *c,
                            size_t d,
                            unsigned int e)
{
    pk(__FUNCTION__);
    return netdev_fo_splice_read_send_req(filp, offset, c, d, e);
}
int     netdev_fo_setlease(
                            struct file *filp,
                            long b,
                            struct file_lock **c)
{
    pk(__FUNCTION__);
    return netdev_fo_setlease_send_req(filp, b, c);
}
long    netdev_fo_fallocate(
                            struct file *filp,
                            int b,
                            loff_t offset,
                            loff_t len)
{
    pk(__FUNCTION__);
    return netdev_fo_fallocate_send_req(filp, b, offset, len);
}
int netdev_fo_show_fdinfo(
                            struct seq_file *a,
                            struct file *filp)
{
    pk(__FUNCTION__);
    return netdev_fo_show_fdinfo_send_req(a, filp);
}

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

static int __init netdev_init(void) /* Constructor */
{
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

static void __exit netdev_exit(void) /* Destructor */
{
    // first diable the netlink module
    netlink_exit();
    // then you can disable the device
    /* TODO this should be really done by netlink code since it will disable devices
     * base on establised connections with netdev server process */
    netdev_cleanup();
    return;
}


module_exit(netdev_exit);
module_init(netdev_init);

#include "fo.h"
#include <linux/sched.h>    /* access to current->comm and current->pid */
#include <linux/module.h>

#include "netdevmgm.h"
#include "fo_send.h"

static void pk(const char * name) {
    printk(KERN_INFO "netdev: File Operation called by \"%s\", PID: %d - %s\n",
            current->comm,
            current->pid,
            name);
}

loff_t  ndfo_llseek ( struct file *filp, loff_t offset, int whence) {
    pk(__FUNCTION__);
    return ndfo_send_llseek(filp, offset, whence);
}
ssize_t ndfo_read ( struct file *filp, char __user *data, size_t c, loff_t *offset) {
    pk(__FUNCTION__);
    return ndfo_send_read(filp, data, c, offset);
}
ssize_t ndfo_write ( struct file *filp, const char __user *data, size_t c, loff_t *offset) {
    pk(__FUNCTION__);
    return ndfo_send_write(filp, data, c, offset);
}
ssize_t ndfo_aio_read ( struct kiocb *a, const struct iovec *b, unsigned long c, loff_t offset) {
    pk(__FUNCTION__);
    return ndfo_send_aio_read(a, b ,c, offset);
}
ssize_t ndfo_aio_write ( struct kiocb *a, const struct iovec *b, unsigned long c, loff_t offset) {
    pk(__FUNCTION__);
    return ndfo_send_aio_write(a, b, c, offset);
}
unsigned int ndfo_poll ( struct file *filp, struct poll_table_struct *wait) {
    pk(__FUNCTION__);
    return ndfo_send_poll(filp, wait);
}
long    ndfo_unlocked_ioctl ( struct file *filp, unsigned int b, unsigned long c) {
    pk(__FUNCTION__);
    return ndfo_send_unlocked_ioctl(filp, b, c);
}
long    ndfo_compat_ioctl ( struct file *filp, unsigned int b, unsigned long c) {
    pk(__FUNCTION__);
    return ndfo_send_compat_ioctl(filp, b, c);
}
int     ndfo_mmap ( struct file *filp, struct vm_area_struct *b) {
    pk(__FUNCTION__);
    return ndfo_send_mmap(filp, b);
}
int     ndfo_open ( struct inode *inode, struct file *filp) {
    struct netdev_data *nddata;

    pk(__FUNCTION__);
    
    /* get the device connected with this file */
    nddata = ndmgm_find(netdev_minors_used[MINOR(inode->i_cdev->dev)]);

    /* set private data for easy access to netdev_data struct */
    filp->private_data = (void*)nddata;

    return ndfo_send_open(inode, filp);
}
int     ndfo_flush ( struct file *filp, fl_owner_t id) {
    pk(__FUNCTION__);
    return ndfo_send_flush(filp, id);
}
int     ndfo_release ( struct inode *a, struct file *b) {
    pk(__FUNCTION__);
    return ndfo_send_release(a, b);
}
int     ndfo_fsync ( struct file *filp, loff_t b, loff_t c, int d) {
    pk(__FUNCTION__);
    return ndfo_send_fsync(filp, b, c, d);
}
int     ndfo_aio_fsync ( struct kiocb *a, int b) {
    pk(__FUNCTION__);
    return ndfo_send_aio_fsync(a, b);
}
int     ndfo_fasync ( int a, struct file *b, int c) {
    pk(__FUNCTION__);
    return ndfo_send_fasync(a, b, c);
}
int     ndfo_lock ( struct file *filp, int b, struct file_lock *c) {
    pk(__FUNCTION__);
    return ndfo_send_lock(filp, b, c);
}
ssize_t ndfo_sendpage ( struct file *filp, struct page *b, int c, size_t d, loff_t *offset, int f) {
    pk(__FUNCTION__);
    return ndfo_send_sendpage(filp, b, c, d, offset, f);
}
unsigned long ndfo_get_unmapped_area( struct file *filp, unsigned long b, unsigned long c, unsigned long d, unsigned long e) {
    pk(__FUNCTION__);
    return ndfo_send_get_unmapped_area(filp, b, c, d, e);
}
int     ndfo_check_flags( int a) {
    pk(__FUNCTION__);
    return ndfo_send_check_flags(a);
}
int     ndfo_flock ( struct file *filp, int b, struct file_lock *c) {
    pk(__FUNCTION__);
    return ndfo_send_flock(filp, b, c);
}
ssize_t ndfo_splice_write( struct pipe_inode_info *a, struct file *filp, loff_t *offset, size_t d, unsigned int e) {
    pk(__FUNCTION__);
    return ndfo_send_splice_write(a, filp, offset, d, e);
}
ssize_t ndfo_splice_read( struct file *filp, loff_t *offset, struct pipe_inode_info *c, size_t d, unsigned int e) {
    pk(__FUNCTION__);
    return ndfo_send_splice_read(filp, offset, c, d, e);
}
int     ndfo_setlease( struct file *filp, long b, struct file_lock **c) {
    pk(__FUNCTION__);
    return ndfo_send_setlease(filp, b, c);
}
long    ndfo_fallocate( struct file *filp, int b, loff_t offset, loff_t len) {
    pk(__FUNCTION__);
    return ndfo_send_fallocate(filp, b, offset, len);
}
int ndfo_show_fdinfo( struct seq_file *a, struct file *filp) {
    pk(__FUNCTION__);
    return ndfo_send_show_fdinfo(a, filp);
}

/* TODO this struct will have to be generated dynamically based on the 
 * operations supported by the device on the other end */
struct file_operations netdev_fops = {
    .owner             = THIS_MODULE,
    .llseek            = ndfo_llseek,
    .read              = ndfo_read,
    .write             = ndfo_write,
    .aio_read          = NULL, /* will be hard to implement because of kiocb */
    .aio_write         = NULL, /* will be hard to implement because of kiocb */
    .readdir           = NULL, /* useless for devices */
    .poll              = ndfo_poll,/* NULL assumes it's not blocing */
    .unlocked_ioctl    = ndfo_unlocked_ioctl,
    .compat_ioctl      = ndfo_compat_ioctl,
    .mmap              = ndfo_mmap,
    .open              = ndfo_open,
    .flush             = ndfo_flush, /* rarely used for devices */
    .release           = ndfo_release,
    .fsync             = ndfo_fsync,
    .aio_fsync         = NULL, /* will be hard to implement because of kiocb */
    .fasync            = ndfo_fasync,
    .lock              = ndfo_lock, /* almost never implemented */
    .sendpage          = ndfo_sendpage, /* usually not used */
    .get_unmapped_area = ndfo_get_unmapped_area, /* mostly NULL */
    .check_flags       = ndfo_check_flags, /* fcntl(F_SETFL...) */
    .flock             = ndfo_flock,
    .splice_write      = ndfo_splice_write, /* needs DMA */
    .splice_read       = ndfo_splice_read, /* needs DMA */
    .setlease          = ndfo_setlease,
    .fallocate         = ndfo_fallocate,
    .show_fdinfo       = ndfo_show_fdinfo
};

#include "fo.h"
#include <linux/sched.h>    /* access to current->comm and current->pid */
#include <linux/module.h>

static void pk(const char * name) {
    printk(KERN_INFO "netdev: File Operation called by \"%s\", PID: %d - %s\n",
            current->comm,
            current->pid,
            name);
}

loff_t  netdev_fo_llseek ( struct file *filp, loff_t offset, int whence) {
    pk(__FUNCTION__);
    return netdev_fo_llseek_send_req(filp, offset, whence);
}
ssize_t netdev_fo_read ( struct file *filp, char __user *data, size_t c, loff_t *offset) {
    pk(__FUNCTION__);
    return netdev_fo_read_send_req(filp, data, c, offset);
}
ssize_t netdev_fo_write ( struct file *filp, const char __user *data, size_t c, loff_t *offset) {
    pk(__FUNCTION__);
    return netdev_fo_write_send_req(filp, data, c, offset);
}
ssize_t netdev_fo_aio_read ( struct kiocb *a, const struct iovec *b, unsigned long c, loff_t offset) {
    pk(__FUNCTION__);
    return netdev_fo_aio_read_send_req(a, b ,c, offset);
}
ssize_t netdev_fo_aio_write ( struct kiocb *a, const struct iovec *b, unsigned long c, loff_t offset) {
    pk(__FUNCTION__);
    return netdev_fo_aio_write_send_req(a, b, c, offset);
}
unsigned int netdev_fo_poll ( struct file *filp, struct poll_table_struct *wait) {
    pk(__FUNCTION__);
    return netdev_fo_poll_send_req(filp, wait);
}
long    netdev_fo_unlocked_ioctl ( struct file *filp, unsigned int b, unsigned long c) {
    pk(__FUNCTION__);
    return netdev_fo_unlocked_ioctl_send_req(filp, b, c);
}
long    netdev_fo_compat_ioctl ( struct file *filp, unsigned int b, unsigned long c) {
    pk(__FUNCTION__);
    return netdev_fo_compat_ioctl_send_req(filp, b, c);
}
int     netdev_fo_mmap ( struct file *filp, struct vm_area_struct *b) {
    pk(__FUNCTION__);
    return netdev_fo_mmap_send_req(filp, b);
}
int     netdev_fo_open ( struct inode *inode, struct file *filp) {
    pk(__FUNCTION__);

    //flip->private_data =
    return netdev_fo_open_send_req(inode, filp);
}
int     netdev_fo_flush ( struct file *filp, fl_owner_t id) {
    pk(__FUNCTION__);
    return netdev_fo_flush_send_req(filp, id);
}
int     netdev_fo_release ( struct inode *a, struct file *b) {
    pk(__FUNCTION__);
    return netdev_fo_release_send_req(a, b);
}
int     netdev_fo_fsync ( struct file *filp, loff_t b, loff_t c, int d) {
    pk(__FUNCTION__);
    return netdev_fo_fsync_send_req(filp, b, c, d);
}
int     netdev_fo_aio_fsync ( struct kiocb *a, int b) {
    pk(__FUNCTION__);
    return netdev_fo_aio_fsync_send_req(a, b);
}
int     netdev_fo_fasync ( int a, struct file *b, int c) {
    pk(__FUNCTION__);
    return netdev_fo_fasync_send_req(a, b, c);
}
int     netdev_fo_lock ( struct file *filp, int b, struct file_lock *c) {
    pk(__FUNCTION__);
    return netdev_fo_lock_send_req(filp, b, c);
}
ssize_t netdev_fo_sendpage ( struct file *filp, struct page *b, int c, size_t d, loff_t *offset, int f) {
    pk(__FUNCTION__);
    return netdev_fo_sendpage_send_req(filp, b, c, d, offset, f);
}
unsigned long netdev_fo_get_unmapped_area( struct file *filp, unsigned long b, unsigned long c, unsigned long d, unsigned long e) {
    pk(__FUNCTION__);
    return netdev_fo_get_unmapped_area_send_req(filp, b, c, d, e);
}
int     netdev_fo_check_flags( int a) {
    pk(__FUNCTION__);
    return netdev_fo_check_flags_send_req(a);
}
int     netdev_fo_flock ( struct file *filp, int b, struct file_lock *c) {
    pk(__FUNCTION__);
    return netdev_fo_flock_send_req(filp, b, c);
}
ssize_t netdev_fo_splice_write( struct pipe_inode_info *a, struct file *filp, loff_t *offset, size_t d, unsigned int e) {
    pk(__FUNCTION__);
    return netdev_fo_splice_write_send_req(a, filp, offset, d, e);
}
ssize_t netdev_fo_splice_read( struct file *filp, loff_t *offset, struct pipe_inode_info *c, size_t d, unsigned int e) {
    pk(__FUNCTION__);
    return netdev_fo_splice_read_send_req(filp, offset, c, d, e);
}
int     netdev_fo_setlease( struct file *filp, long b, struct file_lock **c) {
    pk(__FUNCTION__);
    return netdev_fo_setlease_send_req(filp, b, c);
}
long    netdev_fo_fallocate( struct file *filp, int b, loff_t offset, loff_t len) {
    pk(__FUNCTION__);
    return netdev_fo_fallocate_send_req(filp, b, offset, len);
}
int netdev_fo_show_fdinfo( struct seq_file *a, struct file *filp) {
    pk(__FUNCTION__);
    return netdev_fo_show_fdinfo_send_req(a, filp);
}

/* TODO this struct will have to be generated dynamically based on the 
 * operations supported by the device on the other end */
struct file_operations netdev_fops = {
    .owner             = THIS_MODULE,
    .llseek            = netdev_fo_llseek,
    .read              = netdev_fo_read,
    .write             = netdev_fo_write,
    .aio_read          = netdev_fo_aio_read,
    .aio_write         = netdev_fo_aio_write,
    .readdir           = NULL, /* useless for devices */
    .poll              = netdev_fo_poll,/* NULL assumes it's not blocing */
    .unlocked_ioctl    = netdev_fo_unlocked_ioctl,
    .compat_ioctl      = netdev_fo_compat_ioctl,
    .mmap              = netdev_fo_mmap,
    .open              = netdev_fo_open,
    .flush             = netdev_fo_flush, /* rarely used for devices */
    .release           = netdev_fo_release,
    .fsync             = netdev_fo_fsync,
    .aio_fsync         = netdev_fo_aio_fsync,
    .fasync            = netdev_fo_fasync,
    .lock              = netdev_fo_lock, /* almost never implemented */
    .sendpage          = netdev_fo_sendpage, /* usually not used */
    .get_unmapped_area = netdev_fo_get_unmapped_area, /* mostly NULL */
    .check_flags       = netdev_fo_check_flags, /* fcntl(F_SETFL...) */
    .flock             = netdev_fo_flock,
    .splice_write      = netdev_fo_splice_write, /* needs DMA */
    .splice_read       = netdev_fo_splice_read, /* needs DMA */
    .setlease          = netdev_fo_setlease,
    .fallocate         = netdev_fo_fallocate,
    .show_fdinfo       = netdev_fo_show_fdinfo
};

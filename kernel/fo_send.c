#include "fo.h"
#include "netlink.h"

/* used for sending file operations converted by send_req functions to 
 * a buffer of certian size to the loop sending operations whtough netlink
 * to the server process */
/* TODO might require a struct that will also hold the wait queue to unblock waiting
 * send_req function once receiving loop gets the response */
int send_fo (short fl_flag, const char *data, size_t size) {
    netlink_send(fl_flag);
    return -EIO;
}

/* functions for sending and receiving file operations */
loff_t netdev_fo_llseek_send_req (struct file *flip, loff_t offset, int whence) {
    return -EIO;
}
ssize_t netdev_fo_read_send_req (struct file *flip, char __user *data, size_t c, loff_t *offset) {
    return -EIO;
}
ssize_t netdev_fo_write_send_req (struct file *flip, const char __user *data, size_t c, loff_t *offset) {
    return -EIO;
}
size_t netdev_fo_aio_read_send_req (struct kiocb *a, const struct iovec *b, unsigned long c, loff_t offset) {
    return -EIO;
}
ssize_t netdev_fo_aio_write_send_req (struct kiocb *a, const struct iovec *b, unsigned long c, loff_t d) {
    return -EIO;
}
int netdev_fo_readdir_send_req (struct file *filp, void *b, filldir_t c) {
    return -EIO;
}
unsigned int netdev_fo_poll_send_req (struct file *filp, struct poll_table_struct *wait) {
    return -EIO;
}
long netdev_fo_unlocked_ioctl_send_req (struct file *filp, unsigned int b, unsigned long c) {
    return -EIO;
}
long netdev_fo_compat_ioctl_send_req (struct file *filp, unsigned int b, unsigned long c) {
    return -EIO;
}
int netdev_fo_mmap_send_req (struct file *filp, struct vm_area_struct *b) {
    return -EIO;
}
int netdev_fo_open_send_req (struct inode *inode, struct file *filp) {
    send_fo(MSGTYPE_FO_OPEN, NULL, 0); // Test sending operations
    return 0; /* return success to see other operations */
}
int netdev_fo_flush_send_req (struct file *filp, fl_owner_t id) {
    return 0; /* does nothing, can return success */
}
int netdev_fo_release_send_req (struct inode *a, struct file *filp) {
    return 0; /* return success, no harm done */
}
int netdev_fo_fsync_send_req (struct file *filp, loff_t b, loff_t offset, int d) {
    return -EIO;
}
int netdev_fo_aio_fsync_send_req (struct kiocb *a, int b) {
    return -EIO;
}
int netdev_fo_fasync_send_req (int a, struct file *filp, int c) {
    return -EIO;
}
int netdev_fo_lock_send_req (struct file *filp, int b, struct file_lock *c) {
    return -EIO;
}
ssize_t netdev_fo_sendpage_send_req (struct file *filp, struct page *b, int c, size_t d, loff_t *offset, int f) {
    return -EIO;
}
unsigned long netdev_fo_get_unmapped_area_send_req (struct file *filp, unsigned long b, unsigned long c,unsigned long d, unsigned long e) {
    return -EIO;
}
int netdev_fo_check_flags_send_req (int a) {
    return -EIO;
}
int netdev_fo_flock_send_req (struct file *filp, int b, struct file_lock *c) {
    return -EIO;
}
ssize_t netdev_fo_splice_write_send_req (struct pipe_inode_info *a, struct file *filp, loff_t *offset, size_t d, unsigned int e) {
    return -EIO;
}
ssize_t netdev_fo_splice_read_send_req (struct file *filp, loff_t *offset, struct pipe_inode_info *c, size_t d, unsigned int e) {
    return -EIO;
}
int netdev_fo_setlease_send_req (struct file *filp, long b, struct file_lock **c) {
    return -EIO;
}
long netdev_fo_fallocate_send_req (struct file *filp, int b, loff_t offset, loff_t len) {
    return -EIO;
}
int netdev_fo_show_fdinfo_send_req (struct seq_file *a, struct file *filp) {
    return -EIO;
}

#ifndef _FO_H
#define _FO_H

#include <linux/types.h>
#include <linux/fs.h>

/* used for sending file operations converted by send_req functions to 
 * a buffer of certian size to the loop sending operations whtough netlink
 * to the server process */
int send_fo (short fl_flag, const char *data, size_t size);

/* structures used to pass file operation arguments to sending function */
/* TODO add return values of functions as "rvalue" or something like that */
struct s_fo_llseek {
    struct file *flip;
    loff_t offset;
    int whence;
};
struct s_fo_read {
    struct file *flip;
    char __user *data;
    size_t c;
    loff_t *offset;
};
struct s_fo_write {
    struct file *flip;
    const char __user *data;
    size_t c;
    loff_t *offset;
};
struct s_fo_aio_read {
    struct kiocb *a;
    const struct iovec *b;
    unsigned long c;
    loff_t offset;
};
struct s_fo_aio_write {
    struct kiocb *a;
    const struct iovec *b;
    unsigned long c;
    loff_t offset;
};
struct s_fo_readdir {
    struct file *filp;
    void *b;
    filldir_t c;
};
struct s_dev_fo_poll {
    struct file *filp;
    struct poll_table_struct *wait;
};
struct s_fo_unlocked_ioctl {
    struct file *filp;
    unsigned int b;
    unsigned long c;
};
struct s_fo_compat_ioctl {
    struct file *filp;
    unsigned int b;
    unsigned long c;
};
struct s_fo_mmap {
    struct file *filp;
    struct vm_area_struct *b;
};
struct s_fo_open {
    struct inode *inode;
    struct file *filp;
};
struct s_fo_flush {
    struct file *filp;
    fl_owner_t id;
};
struct s_fo_release {
    struct inode *a;
    struct file *filp;
};
struct s_fo_fsync {
    struct file *filp;
    loff_t b;
    loff_t c;
    int d;
};
struct s_fo_aio_fsync {
    struct kiocb *a;
    int b;
};
struct s_fo_fasync {
    int a;
    struct file *filp;
    int c;
};
struct s_fo_lock {
    struct file *filp;
    int b;
    struct file_lock *c;
};
struct s_fo_sendpage {
    struct file *filp;
    struct page *b;
    int c;
    size_t d;
    loff_t *offset;
    int f;
};
struct s_fo_get_unmapped_area{
    struct file *filp;
    unsigned long b;
    unsigned long c;
    unsigned long d;
    unsigned long e;
};
struct s_fo_check_flags{
    int a;
};
struct s_fo_flock {
    struct file *filp;
    int b;
    struct file_lock *c;
};
struct s_fo_splice_write{
    struct pipe_inode_info *a;
    struct file *filp;
    loff_t *offset;
    size_t d;
    unsigned int e;
};
struct s_fo_splice_read{
    struct file *filp;
    loff_t *b;
    struct pipe_inode_info *c;
    size_t d;
    unsigned int e;
};
struct s_fo_setlease{
    long b;
    struct file *filp;
    struct file_lock **c;
};
struct s_fo_fallocate{
    struct file *filp;
    int b;
    loff_t offset;
    loff_t len;
};
struct s_fo_show_fdinfo{
    struct seq_file *a;
    struct file *filp;
};

/* file operation functions for file_operations structure */
loff_t netdev_fo_llseek (struct file *flip, loff_t offset, int whence);
ssize_t netdev_fo_read (struct file *flip, char __user *data, size_t c, loff_t *offset);
ssize_t netdev_fo_write (struct file *flip, const char __user *data, size_t c, loff_t *offset);
ssize_t netdev_fo_aio_read (struct kiocb *a, const struct iovec *b, unsigned long c, loff_t offset);
ssize_t netdev_fo_aio_write (struct kiocb *a, const struct iovec *b, unsigned long c, loff_t d);
int netdev_fo_readdir (struct file *filp, void *b, filldir_t c);
unsigned int netdev_fo_poll (struct file *filp, struct poll_table_struct *wait);
long netdev_fo_unlocked_ioctl (struct file *filp, unsigned int b, unsigned long c);
long netdev_fo_compat_ioctl (struct file *filp, unsigned int b, unsigned long c);
int netdev_fo_mmap (struct file *filp, struct vm_area_struct *b);
int netdev_fo_open (struct inode *inode, struct file *filp);
int netdev_fo_flush (struct file *filp, fl_owner_t id);
int netdev_fo_release (struct inode *a, struct file *filp);
int netdev_fo_fsync (struct file *filp, loff_t b, loff_t offset, int d);
int netdev_fo_aio_fsync (struct kiocb *a, int b);
int netdev_fo_fasync (int a, struct file *filp, int c);
int netdev_fo_lock (struct file *filp, int b, struct file_lock *c);
ssize_t netdev_fo_sendpage (struct file *filp, struct page *b, int c, size_t d, loff_t *offset, int f);
unsigned long netdev_fo_get_unmapped_area (struct file *filp, unsigned long b, unsigned long c,unsigned long d, unsigned long e);
int netdev_fo_check_flags (int a);
int netdev_fo_flock (struct file *filp, int b, struct file_lock *c);
ssize_t netdev_fo_splice_write (struct pipe_inode_info *a, struct file *filp, loff_t *offset, size_t d, unsigned int e);
ssize_t netdev_fo_splice_read (struct file *filp, loff_t *offset, struct pipe_inode_info *c, size_t d, unsigned int e);
int netdev_fo_setlease (struct file *filp, long b, struct file_lock **c);
long netdev_fo_fallocate (struct file *filp, int b, loff_t offset, loff_t len);
int netdev_fo_show_fdinfo (struct seq_file *a, struct file *filp);

/* functions for sending and receiving file operations */
loff_t netdev_fo_llseek_send_req (struct file *flip, loff_t offset, int whence);
ssize_t netdev_fo_read_send_req (struct file *flip, char __user *data, size_t c, loff_t *offset);
ssize_t netdev_fo_write_send_req (struct file *flip, const char __user *data, size_t c, loff_t *offset);
size_t netdev_fo_aio_read_send_req (struct kiocb *a, const struct iovec *b, unsigned long c, loff_t offset);
ssize_t netdev_fo_aio_write_send_req (struct kiocb *a, const struct iovec *b, unsigned long c, loff_t d);
int netdev_fo_readdir_send_req (struct file *filp, void *b, filldir_t c);
unsigned int netdev_fo_poll_send_req (struct file *filp, struct poll_table_struct *wait);
long netdev_fo_unlocked_ioctl_send_req (struct file *filp, unsigned int b, unsigned long c);
long netdev_fo_compat_ioctl_send_req (struct file *filp, unsigned int b, unsigned long c);
int netdev_fo_mmap_send_req (struct file *filp, struct vm_area_struct *b);
int netdev_fo_open_send_req (struct inode *inode, struct file *filp);
int netdev_fo_flush_send_req (struct file *filp, fl_owner_t id);
int netdev_fo_release_send_req (struct inode *a, struct file *filp);
int netdev_fo_fsync_send_req (struct file *filp, loff_t b, loff_t offset, int d);
int netdev_fo_aio_fsync_send_req (struct kiocb *a, int b);
int netdev_fo_fasync_send_req (int a, struct file *filp, int c);
int netdev_fo_lock_send_req (struct file *filp, int b, struct file_lock *c);
ssize_t netdev_fo_sendpage_send_req (struct file *filp, struct page *b, int c, size_t d, loff_t *offset, int f);
unsigned long netdev_fo_get_unmapped_area_send_req (struct file *filp, unsigned long b, unsigned long c,unsigned long d, unsigned long e);
int netdev_fo_check_flags_send_req (int a);
int netdev_fo_flock_send_req (struct file *filp, int b, struct file_lock *c);
ssize_t netdev_fo_splice_write_send_req (struct pipe_inode_info *a, struct file *filp, loff_t *offset, size_t d, unsigned int e);
ssize_t netdev_fo_splice_read_send_req (struct file *filp, loff_t *offset, struct pipe_inode_info *c, size_t d, unsigned int e);
int netdev_fo_setlease_send_req (struct file *filp, long b, struct file_lock **c);
long netdev_fo_fallocate_send_req (struct file *filp, int b, loff_t offset, loff_t len);
int netdev_fo_show_fdinfo_send_req (struct seq_file *a, struct file *filp);

#endif /* _FO_H */

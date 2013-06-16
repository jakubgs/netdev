#ifndef _FO_H
#define _FO_H

/* Definitions used to distinguis between file operation structures
 * that contain all the functiona arguments, best used with short or __u16
 * Will be either used in nlmsghdr->nlmsg_type or some inner variable */
#define FO_LLSEEK               101
#define FO_READ                 102
#define FO_WRITE                103
#define FO_AIO_READ             104
#define FO_AIO_WRITE            105
#define FO_READDIR              106
#define FO_POLL                 107
#define FO_UNLOCKED_IOCTL       108
#define FO_COMPAT_IOCTL         109
#define FO_MMAP                 110
#define FO_OPEN                 111
#define FO_FLUSH                112
#define FO_RELEASE              113
#define FO_FSYNC                114
#define FO_AIO_FSYNC            115
#define FO_FASYNC               116
#define FO_LOCK                 117
#define FO_SENDPAGE             118
#define FO_GET_UNMAPPED_AREA    119
#define FO_CHECK_FLAGS          120
#define FO_FLOCK                121
#define FO_SPLICE_WRITE         122
#define FO_SPLICE_READ          123
#define FO_SETLEASE             124
#define FO_FALLOCATE            125
#define FO_SHOW_FDINF           126

/* structures used to pass file operation arguments to sending function */
struct s_fo_llseek {
    struct file *flip;
    loff_t b;
    int c;
};
struct s_fo_read {
    struct file *flip;
    char __user *data;
    size_t c;
    loff_t *d;
};
struct s_fo_write {
    struct file *flip;
    const char __user *data;
    size_t c;
    loff_t *d;
};
struct s_fo_aio_read {
    struct kiocb *a;
    const struct iovec *b;
    unsigned long c;
    loff_t d;
};
struct s_fo_aio_write {
    struct kiocb *a;
    const struct iovec *b;
    unsigned long c;
    loff_t d;
};
struct s_fo_readdir {
    struct file *filp;
    void *b;
    filldir_t c;
};
struct s_dev_fo_poll {
    struct file *filp;
    struct poll_table_struct *b;
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
    loff_t *e;
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
    loff_t *c;
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

/* functions for sending and receiving file operations */
loff_t netdev_fo_llseek_send (struct file *flip, loff_t b, int c) {
    return -EIO;
}
ssize_t netdev_fo_read_send (struct file *flip, char __user *data, size_t c, loff_t *d) {
    return -EIO;
}
ssize_t netdev_fo_write_send (struct file *flip, const char __user *data, size_t c, loff_t *d) {
    return -EIO;
}
size_t netdev_fo_aio_read_send (struct kiocb *a, const struct iovec *b, unsigned long c, loff_t d) {
    return -EIO;
}
ssize_t netdev_fo_aio_write_send (struct kiocb *a, const struct iovec *b, unsigned long c, loff_t d) {
    return -EIO;
}
int netdev_fo_readdir_send (struct file *filp, void *b, filldir_t c) {
    return -EIO;
}
unsigned int netdev_fo_poll_send (struct file *filp, struct poll_table_struct *b) {
    return -EIO;
}
long netdev_fo_unlocked_ioctl_send (struct file *filp, unsigned int b, unsigned long c) {
    return -EIO;
}
long netdev_fo_compat_ioctl_send (struct file *filp, unsigned int b, unsigned long c) {
    return -EIO;
}
int netdev_fo_mmap_send (struct file *filp, struct vm_area_struct *b) {
    return -EIO;
}
int netdev_fo_open_send (struct inode *inode, struct file *filp) {
    return 0; /* return success to see other operations */
}
int netdev_fo_flush_send (struct file *filp, fl_owner_t id) {
    return -EIO;
}
int netdev_fo_release_send (struct inode *a, struct file *filp) {
    return -EIO;
}
int netdev_fo_fsync_send (struct file *filp, loff_t b, loff_t c, int d) {
    return -EIO;
}
int netdev_fo_aio_fsync_send (struct kiocb *a, int b) {
    return -EIO;
}
int netdev_fo_fasync_send (int a, struct file *filp, int c) {
    return -EIO;
}
int netdev_fo_lock_send (struct file *filp, int b, struct file_lock *c) {
    return -EIO;
}
ssize_t netdev_fo_sendpage_send (struct file *filp, struct page *b, int c, size_t d, loff_t *e, int f) {
    return -EIO;
}
unsigned long netdev_fo_get_unmapped_area_send (struct file *filp, unsigned long b, unsigned long c,unsigned long d, unsigned long e) {
    return -EIO;
}
int netdev_fo_check_flags_send(int a) {
    return -EIO;
}
int netdev_fo_flock_send(struct file *filp, int b, struct file_lock *c) {
    return -EIO;
}
ssize_t netdev_fo_splice_write_send(struct pipe_inode_info *a, struct file *filp, loff_t *c, size_t d, unsigned int e) {
    return -EIO;
}
ssize_t netdev_fo_splice_read_send(struct file *filp, loff_t *b, struct pipe_inode_info *c, size_t d, unsigned int e) {
    return -EIO;
}
int netdev_fo_setlease_send(struct file *filp, long b, struct file_lock **c) {
    return -EIO;
}
long netdev_fo_fallocate_send(struct file *filp, int b, loff_t offset, loff_t len) {
    return -EIO;
}
int netdev_fo_show_fdinfo_send(struct seq_file *a, struct file *filp) {
    return -EIO;
}


#endif /* _AHCI_H */

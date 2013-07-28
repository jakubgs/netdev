#include <linux/slab.h>

#include "fo.h"
#include "protocol.h"
#include "netlink.h"
#include "fo_struct.h"

/* used for sending file operations converted by send_req functions to
 * a buffer of certian size to the loop sending operations whtough netlink
 * to the server process */
/* TODO might require a struct that will also hold the wait queue to unblock waiting
 * send_req function once receiving loop gets the response */
int send_fo (short fl_flag, struct netdev_data *nddata, void *args, size_t size) {
    int rvalue = 0;
    void *buffer = NULL;
    struct fo_req *req = NULL;

    req = kmem_cache_alloc(nddata->queue_pool, GFP_KERNEL);

    if (!req) {
        printk(KERN_ERR "senf_fo: failed to allocate queue_pool\n");
        return -ENOMEM;
    }

    req->args = args;
    req->size = size;
    init_completion(&req->comp);

    buffer = kzalloc(req->size, GFP_KERNEL);

    if (!buffer) {
        printk(KERN_ERR "send_fo: failed to allocate buffer\n");
        rvalue = -ENOMEM;
        goto out;
    }

    memcpy(req->args, buffer, req->size);

    /* add the req to a queue of requests */
    kfifo_in(&nddata->fo_queue, req, sizeof(*req)); /* TODO semaphore */

    netlink_send(nddata, fl_flag, buffer, req->size);

    /* wait for completion, it will be signaled once a reply is received */
    wait_for_completion(&req->comp);
    rvalue = req->rvalue;

out:
    kmem_cache_free(nddata->queue_pool, req);
    return rvalue;
}

/* functions for sending and receiving file operations */
loff_t netdev_fo_llseek_send_req (struct file *filp, loff_t offset, int whence) {
    return -EIO;
}
ssize_t netdev_fo_read_send_req (struct file *filp, char __user *data, size_t size, loff_t *offset) {
    struct s_fo_read args = {
        .data = NULL,
        .size = size,
        .offset = offset
    };

    if ( send_fo(MSGT_FO_READ, filp->private_data, &args, sizeof(args)) ) {
        return args.rvalue;
    }
    return -EIO;
}
ssize_t netdev_fo_write_send_req (struct file *filp, const char __user *data, size_t c, loff_t *offset) {
    return -EIO;
}
size_t netdev_fo_aio_read_send_req (struct kiocb *a, const struct iovec *b, unsigned long c, loff_t offset) {
    return -EIO;
}
ssize_t netdev_fo_aio_write_send_req (struct kiocb *a, const struct iovec *b, unsigned long c, loff_t d) {
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
    struct s_fo_open args = {
        .inode = inode
    };

    if ( send_fo(MSGT_FO_OPEN, filp->private_data, &args, sizeof(args)) ) {
        return args.rvalue;
    }
    return 0;
}
int netdev_fo_flush_send_req (struct file *filp, fl_owner_t id) {
    send_fo(MSGT_FO_FLUSH, filp->private_data, NULL, 0);
    return 0;
}
int netdev_fo_release_send_req (struct inode *a, struct file *filp) {
    send_fo(MSGT_FO_RELEASE, filp->private_data, NULL, 0);
    return 0;
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

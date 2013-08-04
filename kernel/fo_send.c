#include <linux/slab.h>
#include <linux/kthread.h>

#include "fo.h"
#include "protocol.h"
#include "netlink.h"
#include "fo_struct.h"
#include "dbg.h"

/* used for sending file operations converted by send_req functions to
 * a buffer of certian size to the loop sending operations whtough netlink
 * to the server process */
/* TODO might require a struct that will also hold the wait queue to unblock waiting
 * send_req function once receiving loop gets the response */
int fo_send(short msgtype, struct netdev_data *nddata, void *args, size_t size)
{
    int rvalue = 0;
    void *buffer = NULL;
    struct fo_req *req = NULL;

    /* increment reference counter of netdev_data */
    ndmgm_get(nddata);

    req = kmem_cache_alloc(nddata->queue_pool, GFP_KERNEL);

    if (!req) {
        printk(KERN_ERR "senf_fo: failed to allocate queue_pool\n");
        return -ENOMEM;
    }
    
    req->msgtype = msgtype;
    req->args = args;
    req->size = size;
    init_completion(&req->comp);
    debug("size = %zu", req->size);

    buffer = kzalloc(req->size, GFP_KERNEL);

    if (!buffer) {
        printk(KERN_ERR "fo_send: failed to allocate buffer\n");
        rvalue = -ENOMEM;
        goto out;
    }

    memcpy(req->args, buffer, req->size);

    /* add the req to a queue of requests */
    kfifo_in(&nddata->fo_queue, req, sizeof(*req)); /* TODO semaphore */

    netlink_send_fo(nddata, req);

    /* wait for completion, it will be signaled once a reply is received */
    wait_for_completion(&req->comp);
    debug("returned from wait for file operation");
    rvalue = req->rvalue;
    debug("rvalue = %d", rvalue);

out:
    kmem_cache_free(nddata->queue_pool, req);
    ndmgm_put(nddata); /* decrease references to netdev_data */
    return rvalue;
}

int fo_recv(struct sk_buff *skb)
{
    struct netdev_data *nddata = NULL;
    struct task_struct *task = NULL;
    struct nlmsghdr *nlh = NULL;

    nlh = nlmsg_hdr(skb);

    nddata = ndmgm_find(nlh->nlmsg_pid);
    if (IS_ERR(nddata)) {
        printk(KERN_ERR "fo_execute: failed to find device for pid = %d\n",
                nlh->nlmsg_pid);
        return 1; /* failute */
    }

    if (nddata->dummy) {
        return fo_complete(nddata, nlh, skb); /* find fo in queue and complete it */
    } else {
        /* crate a new thread since we want to return control to the
         * server process so it doesn't wait needlesly */
        task = kthread_run(&fo_execute, (void*)skb, "fo_execute");
        if (IS_ERR(task)) {
            printk("fo_recv: failed to create thread for file operation, errpr = %ld\n", PTR_ERR(task));
            return 1; /* failure */
        }
    }

    return 0; /* success */
}

int fo_complete(
    struct netdev_data *nddata,
    struct nlmsghdr *nlh,
    struct sk_buff *skb)
{
    /* TODO find file operation in the queue, pass result and complete */
    return 1; /* success */
}

/* this function will be executed as a new thread with kthread_run */
int fo_execute(void *data)
{
    struct netdev_data *nddata = NULL;
    struct nlmsghdr *nlh = NULL;
    struct sk_buff *skb = NULL;

    skb = data; /* cast to sk_buff */
    nlh = nlmsg_hdr(skb);

    nddata = ndmgm_find(nlh->nlmsg_pid);
    if (IS_ERR(nddata)) {
        printk(KERN_ERR "fo_execute: failed to find device for pid = %d\n",
                nlh->nlmsg_pid);
        do_exit(-1); /* this is a thread */
    }

    /* TODO execute appropriate file operation and then send back
     * the result to the server in userspace */

    do_exit(0); /* success */
}

/* functions for sending and receiving file operations */
loff_t netdev_fo_llseek_send_req (struct file *filp, loff_t offset, int whence)
{
    return -EIO;
}
ssize_t netdev_fo_read_send_req (struct file *filp, char __user *data, size_t size, loff_t *offset)
{
    struct s_fo_read args = {
        .data = NULL,
        .size = size,
        .offset = offset
    };

    if ( fo_send(MSGT_FO_READ, filp->private_data, &args, sizeof(args)) ) {
        return args.rvalue;
    }
    return -EIO;
}
ssize_t netdev_fo_write_send_req (struct file *filp, const char __user *data, size_t size, loff_t *offset)
{
    struct s_fo_write args = {
        .data = NULL,
        .size = size,
        .offset = offset
    };

    if ( fo_send(MSGT_FO_WRITE, filp->private_data, &args, sizeof(args))) {
        return args.rvalue;
    }
    return -EIO;
}
size_t netdev_fo_aio_read_send_req (struct kiocb *a, const struct iovec *b, unsigned long c, loff_t offset)
{
    return -EIO;
}
ssize_t netdev_fo_aio_write_send_req (struct kiocb *a, const struct iovec *b, unsigned long c, loff_t d)
{
    return -EIO;
}
unsigned int netdev_fo_poll_send_req (struct file *filp, struct poll_table_struct *wait)
{
    return -EIO;
}
long netdev_fo_unlocked_ioctl_send_req (struct file *filp, unsigned int b, unsigned long c)
{
    return -EIO;
}
long netdev_fo_compat_ioctl_send_req (struct file *filp, unsigned int b, unsigned long c)
{
    return -EIO;
}
int netdev_fo_mmap_send_req (struct file *filp, struct vm_area_struct *b)
{
    return -EIO;
}
int netdev_fo_open_send_req (struct inode *inode, struct file *filp)
{
    struct s_fo_open args =
    {
        .inode = inode
    };

    if ( fo_send(MSGT_FO_OPEN, filp->private_data, &args, sizeof(args)) )
    {
        return args.rvalue;
    }
    return 0;
}
int netdev_fo_flush_send_req (struct file *filp, fl_owner_t id)
{
    fo_send(MSGT_FO_FLUSH, filp->private_data, NULL, 0);
    return 0;
}
int netdev_fo_release_send_req (struct inode *a, struct file *filp)
{
    fo_send(MSGT_FO_RELEASE, filp->private_data, NULL, 0);
    return 0;
}
int netdev_fo_fsync_send_req (struct file *filp, loff_t b, loff_t offset, int d)
{
    return -EIO;
}
int netdev_fo_aio_fsync_send_req (struct kiocb *a, int b)
{
    return -EIO;
}
int netdev_fo_fasync_send_req (int a, struct file *filp, int c)
{
    return -EIO;
}
int netdev_fo_lock_send_req (struct file *filp, int b, struct file_lock *c)
{
    return -EIO;
}
ssize_t netdev_fo_sendpage_send_req (struct file *filp, struct page *b, int c, size_t d, loff_t *offset, int f)
{
    return -EIO;
}
unsigned long netdev_fo_get_unmapped_area_send_req (struct file *filp, unsigned long b, unsigned long c,unsigned long d, unsigned long e)
{
    return -EIO;
}
int netdev_fo_check_flags_send_req (int a)
{
    return -EIO;
}
int netdev_fo_flock_send_req (struct file *filp, int b, struct file_lock *c)
{
    return -EIO;
}
ssize_t netdev_fo_splice_write_send_req (struct pipe_inode_info *a, struct file *filp, loff_t *offset, size_t d, unsigned int e)
{
    return -EIO;
}
ssize_t netdev_fo_splice_read_send_req (struct file *filp, loff_t *offset, struct pipe_inode_info *c, size_t d, unsigned int e)
{
    return -EIO;
}
int netdev_fo_setlease_send_req (struct file *filp, long b, struct file_lock **c)
{
    return -EIO;
}
long netdev_fo_fallocate_send_req (struct file *filp, int b, loff_t offset, loff_t len)
{
    return -EIO;
}
int netdev_fo_show_fdinfo_send_req (struct seq_file *a, struct file *filp)
{
    return -EIO;
}

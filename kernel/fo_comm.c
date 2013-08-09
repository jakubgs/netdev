#include <linux/slab.h>
#include <linux/kthread.h>
#include <net/netlink.h>

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
int fo_send(
    short msgtype,
    struct netdev_data *nddata,
    void *args,
    size_t size,
    void *data,
    size_t data_size)
{
    int rvalue = 1;
    size_t bufflen = 0;
    void *buffer = NULL;
    struct fo_req *req = NULL;

    if (!nddata->active) {
        return -EBUSY;
    }

    /* increment reference counter of netdev_data */
    ndmgm_get(nddata);

    req = kmem_cache_alloc(nddata->queue_pool, GFP_KERNEL);

    if (!req) {
        printk(KERN_ERR "senf_fo: failed to allocate queue_pool\n");
        rvalue = -ENOMEM;
        goto out;
    }

    req->msgtype = msgtype;
    req->rvalue = 0;
    req->args = args;
    req->size = size;
    req->data = data;
    req->data_size = data_size;
    init_completion(&req->comp);
    debug("req->size = %zu", req->size);
    debug("req->data_size = %zu", req->data_size);

    /* add the req to a queue of requests */
    ndmgm_foreq_add(nddata, req);

    buffer = fo_serialize(req, &bufflen);
    if (!buffer) {
        printk(KERN_ERR "fo_send: failed to serialize req\n");
        rvalue = -ENODATA;
        goto out;
    }
    debug("bufflen = %zu", bufflen);

    if (!(req->seq = ndmgm_incseq(nddata))) {
        printk(KERN_ERR "fo_send: failed to increment curseq\n");
        rvalue = -EBUSY;
        goto out;
    }
    /* send the file operation request */
    rvalue = netlink_send(nddata,
                            req->seq,
                            req->msgtype,
                            NLM_F_REQUEST,
                            buffer,
                            bufflen);

    if (!rvalue) {
        printk(KERN_ERR "fo_send: failed to send file operation\n");
        rvalue = -ECANCELED;
        goto out;
    }

    /* wait for completion, it will be signaled once a reply is received */
    debug("STARTING WAIT, seq = %ld", req->seq);
    wait_for_completion(&req->comp);
    debug("WAIT COMPLETE, seq = %ld", req->seq);

    rvalue = req->rvalue;
    debug("rvalue = %d", rvalue);
out:
    kmem_cache_free(nddata->queue_pool, req);
    ndmgm_put(nddata);
    return rvalue;
}

int fo_recv(
    struct sk_buff *skb)
{
    struct netdev_data *nddata = NULL;
    struct task_struct *task = NULL;
    struct nlmsghdr *nlh = NULL;
    struct fo_data *data = NULL;
    int rvalue = 0; /* success */

    nlh = nlmsg_hdr(skb);
    skb_pull(skb, sizeof(*nlh));

    nddata = ndmgm_find(nlh->nlmsg_pid);
    if (IS_ERR(nddata)) {
        printk(KERN_ERR "fo_recv: failed to find device for pid = %d\n",
                nlh->nlmsg_pid);
        return 1; /* failure */
    }
    ndmgm_get(nddata);

    if (nddata->dummy) {
        rvalue = fo_complete(nddata, nlh, skb);
    } else {
        if ((data = kzalloc(sizeof(*data), GFP_KERNEL)) == NULL) {
            printk(KERN_ERR "fo_recv: failed to allocate data\n");
            rvalue = 1; /* failure */
        }
        data->nddata = nddata;
        data->nlh = nlh;
        data->skb = skb;

        /* crate a new thread since we want to return control to the
         * server process so it doesn't wait needlesly */
        task = kthread_run(&fo_execute, (void*)data, "fo_execute");
        if (IS_ERR(task)) {
            printk("fo_recv: failed to create thread for file operation, error = %ld\n", PTR_ERR(task));
            rvalue = 1; /* failure */
        }
    }

    ndmgm_put(nddata);
    return rvalue;
}

int fo_complete(
    struct netdev_data *nddata,
    struct nlmsghdr *nlh,
    struct sk_buff *skb)
{
    struct fo_req *req = NULL;
    struct fo_req *recv_req = NULL;

    req = ndmgm_foreq_find(nddata, nlh->nlmsg_seq);
    if (!req) {
        printk(KERN_ERR "fo_complete: failed to obtain fo request\n");
        return 1; /* failure */
    }

    debug("nlh->nlmsg_len = %d", nlh->nlmsg_len);
    debug("payload size = %d", nlmsg_len(nlh));

    recv_req = fo_deserialize(NLMSG_DATA(nlh));
    if (!recv_req) {
        printk(KERN_ERR "fo_complete: failed to deserialize req\n");
        return 1; /* failure */
    }

    /* give arguments and the payload to waiting file operation */
    if (!recv_req->args) {
        memcpy(req->args, recv_req->args, sizeof(req->size));
    }
    if (!recv_req->data) {
        memcpy(req->data, recv_req->data, recv_req->data_size);
    }

    debug("completing file operation, seq = %ld", req->seq);
    complete(&req->comp);
    
    kfree(recv_req);
    return 0; /* success */
}

/* this function will be executed as a new thread with kthread_run */
int fo_execute(
    void *data)
{
    struct fo_data *fodata;
    fodata = (struct fo_data*)data;

    ndmgm_get(fodata->nddata);

    /* TODO copy the skb since netlink_recv will free it */
    /* TODO execute appropriate file operation and then send back
     * the result to the server in userspace */

    ndmgm_put(fodata->nddata);

    do_exit(0); /* success */
}

/* file operation structure:
 * 0 - size_t - size of args structure
 * 1 - size_t - size of data/payload
 * 2 - int    - return value of operation
 * 3 - args   - struct with fo args
 * 4 - data   - payload
 */
void * fo_serialize(
    struct fo_req *req,
    size_t *bufflen)
{
    void *data = NULL;
    size_t size = 0;
    *bufflen =  sizeof(req->size) +
                sizeof(req->data_size) +
                sizeof(req->rvalue) +
                req->size +
                req->data_size;

    data = kzalloc(*bufflen, GFP_KERNEL);
    if (!data) {
        printk(KERN_ERR "fo_serialize: failed to allocate data\n");
        return NULL; /* failure */
    }

    memcpy(data + size, &req->size,      sizeof(req->size));
    size += sizeof(req->size);
    memcpy(data + size, &req->data_size, sizeof(req->data_size));
    size += sizeof(req->data_size);
    memcpy(data + size, &req->rvalue,    sizeof(req->rvalue));
    size += sizeof(req->rvalue);
    memcpy(data + size, req->args,       req->size);
    size += req->size;
    memcpy(data + size, req->data,       req->data_size);

    return data;
}

struct fo_req * fo_deserialize(
    void *data)
{
    struct fo_req *req = NULL;
    size_t size = 0;

    req = kzalloc(sizeof(*req), GFP_KERNEL);
    if (!req) {
        printk(KERN_ERR "fo_deserialize: failed to allocate req\n");
        return NULL; /* failure */
    }

    /* get all the data */
    memcpy(&req->size,      data + size, sizeof(req->size));
    size += sizeof(req->size);
    memcpy(&req->data_size, data + size, sizeof(req->data_size));
    size += sizeof(req->data_size);
    memcpy(&req->rvalue,    data + size, sizeof(req->rvalue));
    size += sizeof(req->rvalue);
    req->args = kzalloc(req->size, GFP_KERNEL);
    if (!req->args) {
        printk(KERN_ERR "fo_deserialize: failed to allocate args\n");
        goto free_req;
    }
    memcpy(req->args,       data + size, req->size);
    size += req->size;
    req->data = kzalloc(req->data_size, GFP_KERNEL);
    if (!req->data) {
        printk(KERN_ERR "fo_deserialize: failed to allocate data\n");
        goto free_req;
    }
    memcpy(req->data,       data + size, req->data_size);

    return req;
free_req:
    kfree(req->data);
    kfree(req->args);
    kfree(req);
    return NULL;
}

/* functions for sending and receiving file operations */
loff_t netdev_fo_llseek_send_req (struct file *filp, loff_t offset, int whence)
{
    return -EIO;
}
ssize_t netdev_fo_read_send_req (struct file *filp, char __user *data, size_t size, loff_t *offset)
{
    struct s_fo_read args = {
        .data = data,
        .size = size,
        .offset = offset
    };

    if ( fo_send(MSGT_FO_READ,
                filp->private_data,
                &args, sizeof(args),
                data, 0) )
    {
        return args.rvalue;
    }
    return -EIO;
}
ssize_t netdev_fo_write_send_req (struct file *filp, const char __user *data, size_t size, loff_t *offset)
{
    struct s_fo_write args = {
        .data = data,
        .size = size,
        .offset = offset
    };

    if ( fo_send(MSGT_FO_WRITE,
                filp->private_data,
                &args, sizeof(args),
                (void*)data, size)) {
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

    if ( fo_send(MSGT_FO_OPEN,
                filp->private_data,
                &args, sizeof(args),
                NULL, 0) )
    {
        return args.rvalue;
    }
    return 0;
}
int netdev_fo_flush_send_req (struct file *filp, fl_owner_t id)
{
    fo_send(MSGT_FO_FLUSH,
            filp->private_data,
            NULL, 0,
            NULL, 0);
    return 0;
}
int netdev_fo_release_send_req (struct inode *a, struct file *filp)
{
    fo_send(MSGT_FO_RELEASE,
            filp->private_data,
            NULL, 0,
            NULL, 0);
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

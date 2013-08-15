#include <linux/slab.h>
#include <linux/kthread.h>
#include <net/netlink.h>
#include <asm/uaccess.h>

#include "protocol.h"
#include "netlink.h"
#include "fo_recv.h"
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
    int rvalue = 0;
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

    req->rvalue = -1; /* assume failure, server has to change it to 0 */
    req->msgtype = msgtype;
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

    if (rvalue < 0) {
        printk(KERN_ERR "fo_send: failed to send file operation\n");
        rvalue = -ECANCELED;
        goto out;
    }

    /* wait for completion, it will be signaled once a reply is received */
    debug("STARTING WAIT, seq = %ld", req->seq);
    wait_for_completion(&req->comp);
    debug("WAIT COMPLETE, seq = %ld", req->seq);

    rvalue = req->rvalue;
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
    void *data = NULL;
    int rvalue = 0; /* success */

    nlh = nlmsg_hdr(skb);

    nddata = ndmgm_find(nlh->nlmsg_pid);
    if (IS_ERR(nddata)) {
        printk(KERN_ERR "fo_recv: failed to find device for pid = %d\n",
                nlh->nlmsg_pid);
        return -1; /* failure */
    }
    ndmgm_get(nddata);

    if (nddata->dummy) {
        rvalue = fo_complete(nddata, nlh, skb);
    } else {
        if ((data = kzalloc(sizeof(*data), GFP_KERNEL)) == NULL) {
            printk(KERN_ERR "fo_recv: failed to allocate data\n");
            rvalue = -1; /* failure */
            goto err;
        }
        /* when fo_recv returns netlink_recv will try to free the skb */
        data = skb_copy(skb, GFP_KERNEL); /* we want to modify it */

        /* crate a new thread since we want to return control to the
         * server process so it doesn't wait needlesly */
        task = kthread_run(&fo_execute, (void*)data, "fo_execute");
        debug("task started!");
        if (IS_ERR(task)) {
            printk("fo_recv: failed to create thread for file operation, error = %ld\n", PTR_ERR(task));
            rvalue = -1; /* failure */
        }
    }

err:
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
    int rvalue = 0;

    req = ndmgm_foreq_find(nddata, nlh->nlmsg_seq);
    if (!req) {
        printk(KERN_ERR "fo_complete: failed to obtain fo request\n");
        return -1; /* failure */
    }

    debug("nlh->nlmsg_len = %d", nlh->nlmsg_len);
    debug("payload size = %d", nlmsg_len(nlh));

    recv_req = fo_deserialize(NLMSG_DATA(nlh));
    if (!recv_req) {
        printk(KERN_ERR "fo_complete: failed to deserialize req\n");
        return -1; /* failure */
    }

    debug("recv_req->rvalue = %d", recv_req->rvalue);
    req->rvalue = recv_req->rvalue;
    /* give arguments and the payload to waiting file operation */
    if (recv_req->args) {
        memcpy(req->args, recv_req->args, recv_req->size);
    }
    if (recv_req->data) {
        debug("access_ok = %ld", access_ok(VERIFY_WRITE, req->data, req->size));
        /* req->data is always in userspace */
        /* TODO this is not working! */
        rvalue = copy_to_user(req->data,
                            recv_req->data,
                            recv_req->data_size);
        if (rvalue > 0) {
            debug("recv_req->data_size = %zu, rvalue = %d",
                    recv_req->data_size, rvalue);
            printk(KERN_ERR "fo_complete: failed to copy to user\n");
            return -1;
        }
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
    int (*fofun)(struct netdev_data*, struct fo_req*) = NULL;
    struct sk_buff *skb = data, *skbtmp;
    struct netdev_data *nddata = NULL;
    struct nlmsghdr *nlh = NULL;
    struct fo_req *req = NULL;
    void *buff = NULL;
    size_t bufflen = 0;
    int fonum = 0;
    int rvalue = -ENODATA;

    nlh = nlmsg_hdr(skb);

    nddata = ndmgm_find(nlh->nlmsg_pid);
    if (IS_ERR(nddata)) {
        printk(KERN_ERR "fo_recv: failed to find device for pid = %d\n",
                nlh->nlmsg_pid);
        goto err;
    }
    ndmgm_get(nddata);

    debug("nlh->nlmsg_len = %d", nlh->nlmsg_len);

    req = fo_deserialize(NLMSG_DATA(nlh));
    if (!req) {
        printk(KERN_ERR "fo_execute: failed to deserialize req\n");
        goto err;
    }

    printk(KERN_INFO "fo_execute: executing file operation = %d\n",
                    nlh->nlmsg_type);

    /* get number of file operation for array */
    fonum = nlh->nlmsg_type - (MSGT_FO_START+1);
    /* get the correct file operation function from the array */
    fofun = netdev_recv_fops[fonum];
    if (!fofun) {
        printk(KERN_ERR "fo_execute: file operation not implemented\n");
        goto err;
    }

    /* execute the file operation */
    req->rvalue = fofun(nddata, req);
    if (req->rvalue == -1) {
        printk(KERN_ERR "fo_execute: file operation failed\n");
        goto err;
    }

    /* send back the result to the server in userspace */
    buff = fo_serialize(req, &bufflen);
    if (!buff) {
        printk(KERN_ERR "fo_execute: failed to serialize req\n");
        goto err;
    }
    debug("bufflen = %zu", bufflen);

    if (bufflen <= nlmsg_len(nlh)) {
        debug("buffer will fit");
        memcpy(nlmsg_data(nlh), buff, bufflen);
    } else {
        debug("buffer won't fit!");
        skbtmp = skb;
        /* expand skb and put in data, possibly split into parts */
        skb = netlink_pack_skb(nlh, buff, bufflen);
        if (!skb) {
            printk(KERN_ERR "fo_execute: failed to put data into skb\n");
            goto err;
        }
        dev_kfree_skb(skbtmp);
    }

    rvalue = 0; /* success */
err:
    /* rvalue is -1 so sending it back untouched will mean failure */
    netlink_send_skb(nddata, skb);
    ndmgm_put(nddata);
    kfree(req);
    kfree(buff);
    do_exit(rvalue);
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
    if (req->args == NULL) {
        debug("no args");
        return data;
    }
    memcpy(data + size, req->args,       req->size);
    size += req->size;
    if (req->data == NULL) {
        debug("no data");
        return data;
    }
    /* req->data is always in user space */
    size = copy_from_user(data + size, req->data, req->data_size);
    if (size > 0) {
        debug("req->data_size = %zu, rvalue = %zu",
                req->size, size);
        printk(KERN_ERR "fo_serialize: failed to copy form user\n");
        kfree(data);
        return NULL;
    }

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
    if (req->size == 0) {
        debug("no args");
        return req;
    }
    size += sizeof(req->rvalue);
    req->args = kzalloc(req->size, GFP_KERNEL);
    if (!req->args) {
        printk(KERN_ERR "fo_deserialize: failed to allocate args\n");
        goto free_req;
    }
    memcpy(req->args,       data + size, req->size);
    if (req->data_size == 0) {
        debug("no data");
        return req;
    }
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

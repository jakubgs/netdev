#include <linux/slab.h>
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

    /* add the req to a queue of requests */
    ndmgm_foreq_add(nddata, req);

    buffer = fo_serialize(req, &bufflen);
    if (!buffer) {
        printk(KERN_ERR "fo_send: failed to serialize req\n");
        rvalue = -ENODATA;
        goto out;
    }

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

/* this function will be executed as a new thread with kthread_run */
int fo_recv(
    void *data)
{
    struct sk_buff *skb = data;
    struct netdev_data *nddata = NULL;
    struct nlmsghdr *nlh = NULL;
    int rvalue = 0; /* success */

    nlh = nlmsg_hdr(skb);

    nddata = ndmgm_find(nlh->nlmsg_pid);
    if (IS_ERR(nddata)) {
        printk(KERN_ERR "fo_recv: failed to find device for pid = %d\n",
                nlh->nlmsg_pid);
        do_exit(-1); /* failure */
    }
    ndmgm_get(nddata);

    if (nddata->dummy) {
        rvalue = fo_complete(nddata, nlh, skb);
    } else {
        rvalue = fo_execute(nddata, nlh, skb);
    }

    ndmgm_put(nddata);
    do_exit(0);
}

int fo_complete(
    struct netdev_data *nddata,
    struct nlmsghdr *nlh,
    struct sk_buff *skb)
{
    struct fo_req *req = NULL;
    struct fo_req *recv_req = NULL;
    int rvalue = -1; /* failure */

    req = ndmgm_foreq_find(nddata, nlh->nlmsg_seq);
    if (!req) {
        printk(KERN_ERR "fo_complete: failed to obtain fo request\n");
        goto err;
    }

    recv_req = fo_deserialize(NLMSG_DATA(nlh));
    if (!recv_req) {
        printk(KERN_ERR "fo_complete: failed to deserialize req\n");
        goto err;
    }

    req->rvalue = recv_req->rvalue;
    /* give arguments and the payload to waiting file operation */
    if (recv_req->args) {
        memcpy(req->args, recv_req->args, recv_req->size);
    }
    if (recv_req->data) {
        memcpy(req->data, recv_req->data, recv_req->data_size);
    }

    complete(&req->comp);
    rvalue = 0; /* success */
err:
    dev_kfree_skb(skb);
    kfree(recv_req);
    return rvalue;
}

int fo_execute(
    struct netdev_data *nddata,
    struct nlmsghdr *nlh,
    struct sk_buff *skb)
{
    int (*fofun)(struct netdev_data*, struct fo_req*) = NULL;
    struct sk_buff *skbtmp = NULL;
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

    if (bufflen <= nlmsg_len(nlh)) {
        memcpy(nlmsg_data(nlh), buff, bufflen);
    } else {
        /* expand skb and put in data, possibly split into parts */
        skbtmp = netlink_pack_skb(nlh, buff, bufflen);
        if (!skb) {
            printk(KERN_ERR "fo_execute: failed to put data into skb\n");
            goto err;
        }
        dev_kfree_skb(skb);
        skb = skbtmp;
    }

    rvalue = 0; /* success */
err:
    schedule(); /* necessary to make sure netlink_recv sends ACK */
    /* rvalue is -1 so sending it back untouched will mean failure */
    netlink_send_skb(nddata, skb);
    ndmgm_put(nddata);
    kfree(req);
    kfree(buff);
    return rvalue;
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
        return data;
    }
    memcpy(data + size, req->args,       req->size);
    size += req->size;
    if (req->data_size == 0) {
        return data;
    }
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
    if (req->size == 0) {
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

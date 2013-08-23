#include <linux/slab.h>
#include <net/netlink.h>
#include <asm/uaccess.h>

#include "protocol.h"
#include "netlink.h"
#include "fo_recv.h"
#include "fo_access.h"
#include "dbg.h"

static void pk(void) {
    printk(KERN_DEBUG "netdev: file operation from:\"%s\", PID: %d - %pS\n",
            current->comm,
            current->pid,
            __builtin_return_address(0));
}

int fo_send(
    short msgtype,
    struct fo_access *acc,
    void *args,
    size_t size,
    void *data,
    size_t data_size)
{
    int rvalue = 0;
    size_t bufflen = 0;
    void *buffer = NULL;
    struct fo_req *req = NULL;

    //pk(); /* for debugging only */

    if (!acc->nddata->active) {
        return -EBUSY;
    }

    /* increment reference counter of netdev_data */
    ndmgm_get(acc->nddata);

    req = kmem_cache_alloc(acc->nddata->queue_pool, GFP_KERNEL);
    if (!req) {
        printk(KERN_ERR "senf_fo: failed to allocate queue_pool\n");
        rvalue = -ENOMEM;
        goto out;
    }

    req->rvalue = -1; /* assume failure, server has to change it to 0 */
    req->msgtype = msgtype;
    req->access_id = acc->access_id;
    req->args = args;
    req->size = size;
    req->data = data;
    req->data_size = data_size;
    init_completion(&req->comp);

    /* add the req to a queue of requests */
    fo_acc_foreq_add(acc, req);

    buffer = fo_serialize(req, &bufflen);
    if (!buffer) {
        printk(KERN_ERR "fo_send: failed to serialize req\n");
        rvalue = -ENODATA;
        goto out;
    }

    if (!(req->seq = ndmgm_incseq(acc->nddata))) {
        printk(KERN_ERR "fo_send: failed to increment curseq\n");
        rvalue = -EBUSY;
        goto out;
    }
    /* send the file operation request */
    rvalue = netlink_send(acc->nddata,
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
    wait_for_completion(&req->comp);

    rvalue = req->rvalue;
out:
    kfree(buffer);
    ndmgm_put(acc->nddata);
    kmem_cache_free(acc->nddata->queue_pool, req);
    if (msgtype == MSGT_FO_RELEASE) {
        fo_acc_destroy(acc);
    }
    return rvalue;
}

/* this function will be executed as a new thread with kthread_run */
int fo_recv(
    void *data)
{
    struct sk_buff *skb = data;
    struct netdev_data *nddata = NULL;
    struct fo_access *acc = NULL;
    struct nlmsghdr *nlh = NULL;
    int msgtype = 0;
    int rvalue = 0; /* success */
    int pid = 0;

    nlh = nlmsg_hdr(skb);
    msgtype = nlh->nlmsg_type;

    nddata = ndmgm_find(nlh->nlmsg_pid);
    if (IS_ERR(nddata)) {
        printk(KERN_ERR "fo_recv: failed to find device for pid = %d\n",
                nlh->nlmsg_pid);
        do_exit(-ENODEV); /* failure */
    }
    ndmgm_get(nddata);

    memcpy(&pid, NLMSG_DATA(nlh), sizeof(pid));
    acc = ndmgm_find_acc(nddata, pid);

    if (!acc) {
        if (msgtype != MSGT_FO_OPEN) {
            printk(KERN_ERR "fo_recv: no opened file for pid = %d\n", pid);
            do_exit(-ENFILE);
        }
        acc = fo_acc_start(nddata, pid);
        if (!acc) {
            printk(KERN_ERR "fo_recv: failed to allocate acc\n");
            do_exit(-ENOMEM);
        }
    }

    if (nddata->dummy) {
        rvalue = fo_complete(acc, nlh, skb);
    } else {
        rvalue = fo_execute(acc, nlh, skb);
    }

    ndmgm_put(nddata);
    do_exit(rvalue);
}

int fo_complete(
    struct fo_access *acc,
    struct nlmsghdr *nlh,
    struct sk_buff *skb)
{
    struct fo_req *req = NULL;
    //debug("completing, pid = %d, seq = %d", acc->access_id, nlh->nlmsg_seq);

    /* first element in paylod is access ID */
    req = fo_acc_foreq_find(acc, nlh->nlmsg_seq);
    if (!req) {
        printk(KERN_ERR "fo_complete: failed to obtain fo request\n");
        return -1;
    }

    if (!fo_deserialize_toreq(req, NLMSG_DATA(nlh))) {
        printk(KERN_ERR "fo_complete: failed to deserialize req\n");
        return -1;
    }

    complete(&req->comp);
    dev_kfree_skb(skb); /* fo_execute frees skb by sending unicat */
    return 0;
}

int fo_execute(
    struct fo_access *acc,
    struct nlmsghdr *nlh,
    struct sk_buff *skb)
{
    int (*fofun)(struct fo_access*, struct fo_req*) = NULL;
    struct sk_buff *skbtmp = NULL;
    struct fo_req *req = NULL;
    void *buff = NULL;
    size_t bufflen = 0;
    int fonum = 0;
    int rvalue = -ENODATA;
    int msgtype = nlh->nlmsg_type;

    req = fo_deserialize(acc, NLMSG_DATA(nlh));
    if (!req) {
        printk(KERN_ERR "fo_execute: failed to deserialize req\n");
        goto err;
    }
    //debug("executing, pid = %d, seq = %d", acc->access_id, nlh->nlmsg_seq);

    /* get number of file operation for array */
    fonum = msgtype - (MSGT_FO_START+1);
    /* get the correct file operation function from the array */
    fofun = netdev_recv_fops[fonum];
    if (!fofun) {
        printk(KERN_ERR "fo_execute: file operation not implemented\n");
        goto err;
    }

    /* execute the file operation */
    req->rvalue = fofun(acc, req);
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
    /* rvalue is -1 so sending it back untouched will mean failure */
    netlink_send_skb(acc->nddata, skb);
    kfree(req->args);
    kfree(req->data);
    kmem_cache_free(acc->nddata->queue_pool, req);
    kfree(buff);
    if (msgtype == MSGT_FO_RELEASE) {
        fo_acc_destroy(acc);
    }
    return rvalue;
}

/* file operation structure:
 * 0 - int    - access ID (HAS TO BE FIRST)
 * 1 - int    - return value of operation
 * 2 - size_t - size of args structure
 * 3 - size_t - size of data/payload
 * 4 - args   - struct with fo args
 * 5 - data   - payload
 */
void * fo_serialize(
    struct fo_req *req,
    size_t *bufflen)
{
    void *data = NULL;
    size_t size = 0;
    *bufflen =  sizeof(req->access_id) +
                sizeof(req->rvalue) +
                sizeof(req->size) +
                sizeof(req->data_size) +
                req->size +
                req->data_size;

    data = kzalloc(*bufflen, GFP_KERNEL);
    if (!data) {
        printk(KERN_ERR "fo_serialize: failed to allocate data\n");
        return NULL; /* failure */
    }

    memcpy(data + size, &req->access_id, sizeof(req->access_id));
    size += sizeof(req->access_id);
    memcpy(data + size, &req->rvalue,    sizeof(req->rvalue));
    size += sizeof(req->rvalue);
    memcpy(data + size, &req->size,      sizeof(req->size));
    size += sizeof(req->size);
    memcpy(data + size, &req->data_size, sizeof(req->data_size));
    size += sizeof(req->data_size);
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

struct fo_req * fo_deserialize_toreq(
    struct fo_req *req,
    void *data)
{
    size_t size = 0;

    if (!req) {
        printk(KERN_ERR "fo_deserialize_toreq: req is NULL\n");
        return NULL; /* failure */
    }

    /* get all the data */
    memcpy(&req->access_id, data + size, sizeof(req->access_id));
    size += sizeof(req->access_id);
    memcpy(&req->rvalue,    data + size, sizeof(req->rvalue));
    size += sizeof(req->rvalue);
    memcpy(&req->size,      data + size, sizeof(req->size));
    size += sizeof(req->size);
    memcpy(&req->data_size, data + size, sizeof(req->data_size));
    if (req->size == 0) {
        return req;
    }
    size += sizeof(req->data_size);
    if (!req->args) {
        req->args = kzalloc(req->size, GFP_KERNEL);
        if (!req->args) {
            printk(KERN_ERR "fo_deserialize_toreq: failed to allocate args\n");
            return NULL;
        }
    }
    memcpy(req->args,       data + size, req->size);
    if (req->data_size == 0) {
        return req;
    }
    size += req->size;
    if (!req->data) {
        req->data = kzalloc(req->data_size, GFP_KERNEL);
        if (!req->data) {
            printk(KERN_ERR "fo_deserialize: failed to allocate data\n");
            return NULL;
        }
    }
    memcpy(req->data,       data + size, req->data_size);

    return req;
}

struct fo_req * fo_deserialize(
    struct fo_access *acc,
    void *data)
{
    struct fo_req *req = NULL;
    size_t size = 0;

    req = kmem_cache_alloc(acc->nddata->queue_pool, GFP_KERNEL);
    if (!req) {
        printk(KERN_ERR "fo_deserialize: failed to allocate req\n");
        return NULL; /* failure */
    }
    req->args = NULL;
    req->data = NULL;

    /* get all the data */
    memcpy(&req->access_id, data + size, sizeof(req->access_id));
    size += sizeof(req->access_id);
    memcpy(&req->rvalue,    data + size, sizeof(req->rvalue));
    size += sizeof(req->rvalue);
    memcpy(&req->size,      data + size, sizeof(req->size));
    size += sizeof(req->size);
    memcpy(&req->data_size, data + size, sizeof(req->data_size));
    if (req->size == 0) {
        return req;
    }
    size += sizeof(req->data_size);
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

#include <asm/errno.h>

#include "fo_access.h"
#include "fo_comm.h"
#include "dbg.h"

struct fo_access * fo_acc_start(struct netdev_data *nddata, int pid) {
    int err = 0;
    struct fo_access *acc = NULL;

    acc = kzalloc(sizeof(*acc), GFP_KERNEL);
    if (!acc) {
        printk(KERN_ERR "fo_acc_alloc: failed to allocate fo_access\n");
        return NULL;
    }

    if (nddata->dummy) { /* server doesn't need a queue */
        if ((err = kfifo_alloc(&acc->fo_queue,
                                NETDEV_FO_QUEUE_SIZE,
                                GFP_KERNEL))) {
            printk(KERN_ERR "fo_acc_alloc: failed to allocate fo_queue\n");
            goto err;
        }
    }

    init_rwsem(&acc->sem);
    acc->access_id = pid;
    acc->nddata = nddata;

    if (down_write_trylock(&nddata->sem)) {
        hash_add(nddata->foacc_htable, &acc->hnode, pid);
        ndmgm_get(nddata);

        up_write(&acc->nddata->sem);
    } else {
        printk(KERN_ERR "fo_acc_start: failed to add acc to hashtable\n");
        goto err;
    }

    return acc;
err:
    if (nddata->dummy) {
        kfifo_free(&acc->fo_queue);
    }
    kfree(acc);
    return NULL;
}

void fo_acc_destroy(
    struct fo_access *acc)
{
    if (down_write_trylock(&acc->nddata->sem)) {
        hash_del(&acc->hnode);
        ndmgm_put(acc->nddata);

        up_write(&acc->nddata->sem);
    }
    if (acc->nddata->dummy) {
        kfifo_free(&acc->fo_queue);
    }
    kfree(acc);
}

int fo_acc_free_queue(
    struct fo_access *acc)
{
    int size = 0;
    struct fo_req *req = NULL;

    /* destroy all requests in the queue */
    while (!kfifo_is_empty(&acc->fo_queue)) {
        size = kfifo_out(&acc->fo_queue, &req, sizeof(req));
        if ( size != sizeof(req) || IS_ERR(req)) {
            printk(KERN_ERR "fo_acc_free_queue: failed to fetch from queue, size = %d\n", size);
            return 1; /* failure */
        }

        req->rvalue = -ENODATA;
        complete(&req->comp); /* complete all pending file operations */
    }

    return 0; /* success */
}

/* find the filo operation request with the correct sequence number */
struct fo_req * fo_acc_foreq_find(
    struct fo_access *acc,
    int seq)
{
    /* will hold the first element as a stopper */
    struct fo_req *req = NULL;
    struct fo_req *tmp = NULL;
    int size = 0;

    if (down_write_trylock(&acc->sem)) {
        while (!kfifo_is_empty(&acc->fo_queue)) {
            size = kfifo_out(&acc->fo_queue, &tmp, sizeof(tmp));
            if (size < sizeof(tmp) || IS_ERR(tmp)) {
                printk(KERN_ERR "fo_acc_foreq_find: failed to get queue element\n");
                tmp = NULL;
                break;
            }
            if (req == NULL) {
                req = tmp; /* first element */
            }
            if (tmp->seq == seq) {
                break;
            }
            /* put the wrong element back into the queue */
            debug("wrong element, putting it back at the end");
            kfifo_in(&acc->fo_queue, &tmp, sizeof(tmp));
            if (req == tmp) {
                tmp = NULL;
                break;
            }
        }
        up_write(&acc->sem);
    }
    return tmp;
}

int fo_acc_foreq_add(
    struct fo_access *acc,
    struct fo_req *req)
{
    if (down_write_trylock(&acc->sem)) {
        kfifo_in(&acc->fo_queue, &req, sizeof(req));
        up_write(&acc->sem);
        return 0; /* success */
    }
    return 1; /* failure */
}



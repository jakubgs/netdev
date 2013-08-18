#ifndef _FO_ACCESS_H
#define _FO_ACCESS_H

#include <linux/kfifo.h>

/* fo_access represents one process with one open file descriptor */
struct fo_access {
    int access_id; /* identification number of process */
    struct netdev_data *nddata; /* netdev device opened by this process */
    struct file *filp; /* file descriptor */
    struct kfifo fo_queue; /* queue for file operations */
    struct rw_semaphore sem; /* necessary since threads can use this */
    struct hlist_node hnode; /* to use with hastable */
};

struct fo_access * fo_acc_start(struct netdev_data *nddata, int pid);
void fo_acc_destroy(struct fo_access *acc);
int fo_acc_free_queue(struct fo_access *acc);
struct fo_req * fo_acc_foreq_find(struct fo_access *acc, int seq);
int fo_acc_foreq_add(struct fo_access *acc, struct fo_req *req);

#endif /* _FO_ACCESS_H */

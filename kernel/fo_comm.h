#ifndef _FO_COMM_H
#define _FO_COMM_H

#include <linux/skbuff.h>
#include <linux/netlink.h>  /* for netlink sockets */
#include <linux/fs.h> /* fl_owner_t */
#include <linux/completion.h>

#include "netdevmgm.h"
#include "fo_access.h"

/* necessary since netdevmgm.h and fo_comm.h reference each other */
struct netdev_data;
/* file structure for a queue of operations of dummy device, those will have
 * to be allocated fast and from a large pool since there will be a lot */
struct fo_req {
    long        seq;        /* for synchronizing netlink messages     */
    short       msgtype;    /* type of file operation                 */
    int         access_id;  /* identification of opened file */
    void       *args;       /* place for s_fo_OPERATION structures    */
    void       *data;       /* place for fo payload */
    size_t      size;       /* size of the s_fo_OPERATION structure   */
    size_t      data_size;
    int         rvalue;     /* informs about success of failure of fo */
    struct completion comp; /* completion to release once reply arrives */
};

int fo_send(short msgtype, struct fo_access *acc, void *args, size_t size, void *data, size_t data_size);
int fo_recv(void *data);
int fo_complete(struct fo_access *acc, struct nlmsghdr *nlh, struct sk_buff *skb);
int fo_execute(struct fo_access *acc, struct nlmsghdr *nlh, struct sk_buff *skb);
void * fo_serialize(struct fo_req *req, size_t *bufflen);
struct fo_req * fo_deserialize_toreq(struct fo_req *req, void *data);
struct fo_req * fo_deserialize(struct fo_access *acc, void *data);

#endif /* _FO_COMM_H */

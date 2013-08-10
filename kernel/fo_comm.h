#ifndef _FO_COMM_H
#define _FO_COMM_H

#include <linux/skbuff.h>
#include <linux/netlink.h>  /* for netlink sockets */
#include <linux/fs.h> /* fl_owner_t */
#include <linux/completion.h>

#include "netdevmgm.h"

struct netdev_data;
/* file structure for a queue of operations of dummy device, those will have
 * to be allocated fast and from a large pool since there will be a lot */
struct fo_req {
    long        seq;       /* for synchronizing netlink messages     */
    short       msgtype;   /* type of file operation                 */
    void       *args;      /* place for s_fo_OPERATION structures    */
    void       *data;      /* place for fo payload */
    size_t      size;      /* size of the s_fo_OPERATION structure   */
    size_t      data_size;
    int         rvalue;    /* informs about success of failure of fo */
    struct completion  comp;     /* completion to release once reply arrives */
};

/* used for sending file operations converted by send_req functions to
 * a buffer of certian size to the loop sending operations whtough netlink
 * to the server process */
int fo_send(short msgtype, struct netdev_data *nddata, void *args, size_t size, void *data, size_t data_size);
int fo_recv(struct sk_buff *skb);
int fo_complete(struct netdev_data *nddata, struct nlmsghdr *nlh, struct sk_buff *skb);
int fo_execute(void *data);
void * fo_serialize(struct fo_req *req, size_t *bufflen);
struct fo_req * fo_deserialize(void *data);

#endif /* _FO_COMM_H */

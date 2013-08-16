#ifndef _NETLINK_H
#define _NETLINK_H

#include <linux/skbuff.h>

#include "netdevmgm.h"
#include "fo.h"

void prnttime(void);
int netlink_echo(int pid, int seq, char *msg);
void netlink_recv(struct sk_buff *skb);
int netlink_create_dummy(int pid, int seq, char *buff, size_t bsize);
int netlink_ackmsg(struct netdev_data *nddata, int seq, short msgtype);
int netlink_send(struct netdev_data *nddata, int seq, short msgtype, int flags, void *buff, size_t bufflen);
int netlink_send_skb(struct netdev_data *nddata, struct sk_buff *skb);
struct sk_buff * netlink_pack_skb( struct nlmsghdr *nlh, void *buff, size_t bufflen);
int netlink_send_fo(struct netdev_data *nddata, struct fo_req *req);
int netlink_init(void);
void netlink_exit(void);

#endif /* _NETLINK_H */

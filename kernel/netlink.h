#ifndef _NETLINK_H
#define _NETLINK_H

#include <linux/skbuff.h>

#include "netdevmgm.h"
#include "fo.h"

int netlink_echo(int pid, char *msg);
void netlink_recv(struct sk_buff *skb);
int netlink_send(struct netdev_data *nddata, short msgtype, char *buff, size_t bufflen);
int netlink_send_fo(struct netdev_data *nddata, struct fo_req *req);
int netlink_init(void);
void netlink_exit(void);

#endif /* _NETLINK_H */

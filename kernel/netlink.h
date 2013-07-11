#ifndef _NETLINK_H
#define _NETLINK_H

#include <net/sock.h>
#include <linux/netlink.h>  /* for netlink sockets */
#include <linux/skbuff.h>

void netlink_recv(struct sk_buff *skb);
int netlink_send(short msgtype);
int netlink_init(void);
void netlink_exit(void);

#endif /* _NETLINK_H */

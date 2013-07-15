#ifndef _NETLINK_H
#define _NETLINK_H

#include <linux/skbuff.h>

int netlink_echo(int pid, char *msg);
void netlink_recv(struct sk_buff *skb);
int netlink_send(short msgtype);
int netlink_init(void);
void netlink_exit(void);

#endif /* _NETLINK_H */

#ifndef _NETLINK_H
#define _NETLINK_H

static void hello_nl_recv_msg(struct sk_buff *skb);
int init_netlink(void);
void exit_netlink(void);

#endif /* _NETLINK_H */

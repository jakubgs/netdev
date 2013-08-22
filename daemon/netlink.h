#ifndef _NETLINK_H
#define _NETLINK_H

#include "proxy.h"

int netlink_setup(struct proxy_dev *pdev);
int netlink_reg_dummy_dev(struct proxy_dev *pdev);
int netlink_reg_remote_dev(struct proxy_dev *pdev);
int netlink_send(struct proxy_dev *pdev, struct nlmsghdr *nlh);
int netlink_send_msg(struct proxy_dev *pdev, void *buff, size_t bsize, int msgtype, int flags);
int netlink_send_nlh(struct proxy_dev *pdev, struct nlmsghdr *nlh);
struct nlmsghdr * netlink_recv(struct proxy_dev *pdev);

#endif

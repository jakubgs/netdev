#ifndef _NETLINK_H
#define _NETLINK_H

#include "proxy.h"

struct netlink_message {
    int msgtype;
    size_t size;
    void * payload;
};

int netlink_setup(struct proxy_dev *pdev);
int netlink_reg_dummy_dev(struct proxy_dev *pdev);
int netlink_reg_remote_dev(struct proxy_dev *pdev);
int netlink_send( struct proxy_dev *pdev, void *buff, size_t bsize, int msgtype, int flags);
struct netlink_message * netlink_recv(struct proxy_dev *pdev);

#endif

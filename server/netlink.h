#ifndef _NETLINK_H
#define _NETLINK_H

#include "proxy.h"

int netlink_setup(struct proxy_dev *pdev);
int netlink_reg_dummy_dev(struct proxy_dev *pdev);
int netlink_reg_remote_dev(struct proxy_dev *pdev);

#endif

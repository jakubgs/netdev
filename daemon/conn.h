#ifndef _CONN_H
#define _CONN_H

#include "proxy.h"
#include "netprotocol.h"

int conn_send(struct proxy_dev *pdev, struct netdev_header *ndhead);
struct netdev_header * conn_recv(struct proxy_dev *pdev);
int conn_server(struct proxy_dev *pdev);
int conn_client(struct proxy_dev *pdev);
int conn_send_dev_reg(struct proxy_dev *pdev);
int conn_recv_dev_reg(struct proxy_dev *pdev);
int recvall(int conn_fd, struct msghdr *msgh, int size);
int sendall(int conn_fd, struct msghdr *msgh, int size);

#endif

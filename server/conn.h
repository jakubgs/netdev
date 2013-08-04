#ifndef _CONN_H
#define _CONN_H

#include "proxy.h"

int conn_server(struct proxy_dev *pdev);
int conn_client(struct proxy_dev *pdev);
int conn_send_version(struct proxy_dev *pdev);
int conn_send_dev_reg(struct proxy_dev *pdev);
int conn_recv_version(struct proxy_dev *pdev);
int conn_recv_dev_reg(struct proxy_dev *pdev);

#endif

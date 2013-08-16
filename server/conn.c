#include <stdio.h> /* perror */
#include <stdlib.h> /* malloc */
#include <arpa/inet.h> /* inet_pton */

#include "conn.h"
#include "protocol.h"
#include "netlink.h"
#include "debug.h"

int conn_send(struct proxy_dev *pdev, struct netdev_header *ndhead) {
    struct msghdr msgh = {0};
    struct iovec iov = {0};
    size_t size = sizeof(*ndhead) + ndhead->size;
    void *buff = NULL;

    debug("sending bytes = %zu", size);

    buff = malloc(size);
    if (!buff) {
        perror("conn_send(malloc)");
        return -1;
    }
    msgh.msg_iov = &iov;
    msgh.msg_iovlen = 1;
    iov.iov_len = size;
    iov.iov_base = buff;
    if (!iov.iov_base) {
        perror("conn_send(malloc)");
        return -1;
    }

    memcpy(iov.iov_base, ndhead, sizeof(*ndhead));
    memcpy(iov.iov_base + sizeof(*ndhead), ndhead->payload, ndhead->size);

    if (sendall(pdev->rm_fd, &msgh, size) == -1) {
        printf("conn_send: failed to send data\n");
        return -1; /* failure */
    }

    free(buff);

    return 0; /* success */
}

struct netdev_header * conn_recv(struct proxy_dev *pdev) {
    struct netdev_header *ndhead = NULL;
    struct msghdr msgh = {0};
    struct iovec iov = {0};
    int ndsize = sizeof(*ndhead);

    ndhead = malloc(ndsize);

    msgh.msg_iov = &iov;
    msgh.msg_iovlen = 1;
    iov.iov_base = (void *)ndhead;
    iov.iov_len = ndsize;

    if (recvall(pdev->rm_fd, &msgh, ndsize) == -1) {
        printf("conn_recv: failed to read message header\n");
        return NULL;
    }
    debug("paylod size = %zu", ndhead->size);

    ndhead->payload = malloc(ndhead->size);
    iov.iov_base = ndhead->payload;
    iov.iov_len = ndhead->size;

    if (recvall(pdev->rm_fd, &msgh, ndhead->size) == -1) {
        printf("conn_recv: failed to read rest of the message\n");
        free(ndhead);
        return NULL;
    }

    return ndhead;
}

int conn_server(struct proxy_dev *pdev) {
    struct netdev_header *ndhead;
    int version = NETDEV_PROTOCOL_VERSION;
    int rvalue = 0;

    /* create TCP/IP socket for connection with server */
    pdev->rm_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (pdev->rm_fd == -1) {
        perror("conn_server(socket)");
        return -1;
    }

    pdev->rm_addr = malloc(sizeof(*pdev->rm_addr));
    if (!pdev->rm_addr) {
        perror("conn_server(malloc)");
        return -1;
    }

    pdev->rm_addr->sin_family = AF_INET;
    pdev->rm_addr->sin_port = htons(pdev->rm_portaddr);
    inet_pton(AF_INET, (void*)pdev->rm_ipaddr, &pdev->rm_addr->sin_addr);

    printf("conn_server: connecting to: %s:%d\n",
            pdev->rm_ipaddr, pdev->rm_portaddr);

    rvalue = connect(pdev->rm_fd,
                    (struct sockaddr*)pdev->rm_addr,
                    sizeof(*pdev->rm_addr));
    if (rvalue == -1) {
        perror("conn_server(connect)");
        return -1;
    }

    ndhead = malloc(sizeof(*ndhead));
    if (!ndhead) {
        perror("conn_server(malloc)");
        return -1;
    }
    ndhead->msgtype = MSGT_CONTROL_VERSION;
    ndhead->size    = sizeof(version);
    ndhead->payload = malloc(ndhead->size);
    memcpy(ndhead->payload, &version, ndhead->size);

    /* send version */
    if (conn_send(pdev, ndhead) == -1) {
        printf("conn_server: failed to send version\n");
        return -1;
    }
    free(ndhead->payload);
    free(ndhead);
    /* receive reply */
    ndhead = conn_recv(pdev);

    if (!ndhead) {
        printf("conn_server: failed to receive version reply\n");
        return -1;
    }

    if (ndhead->size != sizeof(int)) {
        printf("conn_server: wrong payload size\n");
        return -1;
    }

    version = *((int*)ndhead->payload);
    if (version != 0) {
        printf("conn_server: wrong version = %d\n", version);
        return -1;
    }

    printf("conn_server: connection successful, version correct = %d\n",
            version);
    return 0;
}

int conn_client(struct proxy_dev *pdev) {
    struct netdev_header *ndhead;
    int version;
    int reply = NETDEV_PROTOCOL_SUCCESS; /* success = 0 */

    ndhead = conn_recv(pdev);

    if (ndhead->msgtype != MSGT_CONTROL_VERSION) {
        printf("conn_client: wrong message type!\n");
        return -1; /* failure */
    }

    if (ndhead->size != sizeof(int)) {
        printf("conn_client: wrong payload size!\n");
        return -1; /* failure */
    }

    version = *((int*)ndhead->payload);
    printf("conn_client: received version = %d\n", version);
    /* check version number */
    if (version != NETDEV_PROTOCOL_VERSION) {
        printf("conn_client: wrong protocol version = %d\n", version);
        reply = NETDEV_PROTOCOL_VERSION;
    }

    memcpy(ndhead->payload, &reply, sizeof(int));

    debug("sending reply");
    if (conn_send(pdev, ndhead) == -1) {
        printf("conn_client: failed to send version reply\n");
        return -1;
    }

    printf("conn_client: protocol version correct = %d\n", version);
    return reply; /* success */
}

int conn_send_dev_reg(struct proxy_dev *pdev) {
    struct netdev_header *ndhead;
    int rvalue = 0;

    ndhead = malloc(sizeof(*ndhead));
    if (!ndhead) {
        perror("conn_send_dev_reg(malloc)");
        return -1;
    }

    ndhead->msgtype = MSGT_CONTROL_REG_SERVER;
    ndhead->size    = strlen(pdev->remote_dev_name)+1; /* plus null */
    ndhead->payload = pdev->remote_dev_name; /* TODO */

    printf("conn_send_dev_reg: sending device name: %s\n",
            pdev->remote_dev_name);
    if (conn_send(pdev, ndhead) == -1) {
        printf("conn_send_dev_reg: failed to send device registration\n");
        free(ndhead);
        return -1;
    }

    free(ndhead);
    ndhead = conn_recv(pdev);

    if (!ndhead) {
        printf("conn_send_dev_reg: failed to receive reply\n");
        return -1;
    }

    rvalue = *((int*)ndhead->payload);

    if (rvalue != NETDEV_PROTOCOL_SUCCESS) {
        printf("conn_send_dev_reg: server failed to register device\n");
        return -1;
    }

    if (netlink_reg_dummy_dev(pdev) == -1) {
        printf("proxy_client: failed to register device on the server!\n");
        return -1;
    }
    printf("conn_send_dev_reg: device registered\n");
    return 0; /* success */
}

int conn_recv_dev_reg(struct proxy_dev *pdev) {
    struct netdev_header *ndhead;
    int reply = NETDEV_PROTOCOL_SUCCESS;

    ndhead = conn_recv(pdev);

    if (ndhead->msgtype != MSGT_CONTROL_REG_SERVER) {
        printf("conn_recv_dev_reg: wrong message type!\n");
        return -1; /* failure */
    }

    pdev->remote_dev_name = malloc(ndhead->size);
    if (!pdev->remote_dev_name) {
        perror("conn_recv_dev_reg(malloc)");
        return -1;
    }
    memcpy(pdev->remote_dev_name, ndhead->payload, ndhead->size);

    debug("registering device: %s", pdev->remote_dev_name);
    if (netlink_reg_remote_dev(pdev) == -1) {
        printf("conn_recv_dev_reg: failed to register device: %s\n",
                pdev->remote_dev_name);
        return -1;
    }

    ndhead->payload = malloc(sizeof(reply));
    memcpy(ndhead->payload, &reply, sizeof(reply));

    if (conn_send(pdev, ndhead) == -1) {
        printf("conn_recv_dev_reg: failed to send reply\n");
        return -1;
    }

    free(ndhead->payload);
    return 0; /* success */
}

/* size is the minimum that should be read */
int recvall(int conn_fd, struct msghdr *msgh, int size) {
    int rvalue = 0, bytes = 0;

    do {
        rvalue = recvmsg(conn_fd, msgh, msgh->msg_flags);
        //debug("rvalue = %d", rvalue);
        if (rvalue == -1) {
            perror("recvall(recvmsg)");
            return -1; /* falure */
        } else if (rvalue == 0 ) {
            return -1;
        }
        bytes += rvalue;
        msgh->msg_iov->iov_base += rvalue;
        msgh->msg_iov->iov_len = bytes;
    } while (bytes < size);

    return bytes; /* success */
}

int sendall(int conn_fd, struct msghdr *msgh, int size) {
    int rvalue = 0, bytes = 0;

    do {
        rvalue = sendmsg(conn_fd, msgh, msgh->msg_flags | MSG_DONTWAIT);
        //debug("rvalue = %d", rvalue);
        if (rvalue == -1) {
            perror("sendmsg(sendmsg)");
            return -1; /* falure */
        } else if (rvalue == 0 ) {
            return -1;
        }
        bytes += rvalue;
        msgh->msg_iov->iov_base += rvalue;
        msgh->msg_iov->iov_len = bytes;
    } while (bytes < size);

    return 0; /* success */
}

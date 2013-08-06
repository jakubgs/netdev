#include <stdio.h> /* perror */
#include <stdlib.h> /* malloc */
#include <arpa/inet.h> /* inet_pton */

#include "conn.h"
#include "protocol.h"
#include "debug.h"

int conn_send(struct proxy_dev *pdev, struct netdev_header *ndhead) {
    struct msghdr msgh = {0};
    struct iovec iov = {0};
    int size = sizeof(*ndhead) + ndhead->size;

    debug("sending bytes = %zu", ndhead->size);

    msgh.msg_iov = &iov;
    msgh.msg_iovlen = 1;
    iov.iov_len = size;
    iov.iov_base = malloc(size);
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

    free(iov.iov_base);

    debug("success");
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
    debug("received bytes = %zu", ndhead->size);

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

    printf("conn_server: protocol version correct = %d\n", version);
    return 0;
}

int conn_client(struct proxy_dev *pdev) {
    struct netdev_header *ndhead;
    int version;
    int reply = NETDEV_PROTOCOL_CORRECT; /* success = 0 */

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
    /* check version number */
    if (version != NETDEV_PROTOCOL_VERSION) {
        printf("conn_client: wrong protocol version = %d\n", version);
        reply = NETDEV_PROTOCOL_VERSION;
    }

    memcpy(ndhead->payload, &reply, sizeof(int));

    if (conn_send(pdev, ndhead) == -1) {
        printf("conn_client: failed to send version reply\n");
        return -1;
    }

    printf("conn_client: protocol version correct = %d\n", version);
    return reply; /* success */
}

int conn_send_dev_reg(struct proxy_dev *pdev) {
    struct netdev_header ndhead;

    ndhead.msgtype = MSGT_CONTROL_REG_SERVER;
    ndhead.size    = strlen(pdev->remote_dev_name);
    ndhead.payload = pdev->remote_dev_name;

    return conn_send(pdev, &ndhead);
}

int conn_recv_dev_reg(struct proxy_dev *pdev) {
    struct netdev_header *ndhead;

    ndhead = conn_recv(pdev);

    if (ndhead->msgtype != MSGT_CONTROL_REG_SERVER) {
        printf("conn_recv_dev_reg: wrong message type!\n");
        return -1; /* failure */
    }

/* TODO */

    return 0; /* success */
}

/* size is the minimum that should be read */
int recvall(int conn_fd, struct msghdr *msgh, int size) {
    int rvalue = 0, bytes = 0;
    int i = 0;
    printf("recvall: receiving bytes = %d\n", size);

    do {
        rvalue = recvmsg(conn_fd, msgh, 0);
        fprintf(stderr, "recvmsg: rvalue = %d\n", rvalue);
        if (rvalue == -1) {
            perror("recvall(recvmsg)");
            return -1; /* falure */
        }
        bytes += rvalue;
        if (i++ == 10) {
            printf("recvall: pointless\n");
            return -1;
        }
    } while (bytes < size);

    return 0; /* success */
}

int sendall(int conn_fd, struct msghdr *msgh, int size) {
    int rvalue = 0, bytes = 0;
    int i = 0;
    printf("sendall: sending bytes = %d\n", size);

    do {
        rvalue = sendmsg(conn_fd, msgh, 0);
        fprintf(stderr, "sendall: rvalue = %d\n", rvalue);
        if (rvalue == -1) {
            perror("sendmsg(sendmsg)");
            return -1; /* falure */
        }
        bytes += rvalue;
        if (i++ == 10) {
            printf("sendall: pointless\n");
            return -1;
        }
    } while (bytes != size);

    return 0; /* success */
}

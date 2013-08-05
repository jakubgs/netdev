#include <stdio.h> /* perror */
#include <stdlib.h> /* malloc */

#include "conn.h"
#include "protocol.h"

int conn_send(struct proxy_dev *pdev, struct netdev_header *ndhead) {
    struct msghdr msgh = {0};
    struct iovec iov = {0};

    msgh.msg_iov = &iov;
    iov.iov_base = ndhead;
    iov.iov_len = ndhead->size;

    if (!sendall(pdev->rm_fd, &msgh, ndhead->size)) {
        printf("conn_send: failed to send data\n");
        return 0; /* failure */
    }

    return 1; /* success */
}

struct netdev_header * conn_recv(struct proxy_dev *pdev) {
    struct netdev_header *ndhead = NULL;
    struct msghdr msgh = {0};
    struct iovec iov = {0};
    int ndsize = sizeof(*ndhead);

    ndhead = malloc(ndsize);
    
    msgh.msg_iov = &iov;
    iov.iov_base = (void *)ndhead;
    iov.iov_len = ndsize;

    if (!recvall(pdev->rm_fd, &msgh, ndsize)) {
        printf("conn_recv: failed to read message header\n");
        goto err;
    }

    ndhead->payload = malloc(ndhead->size);
    iov.iov_base = ndhead->payload;
    iov.iov_len = ndhead->size;

    if (!recvall(pdev->rm_fd, &msgh, ndhead->size)) {
        printf("conn_recv: failed to read rest of the message\n");
        goto err;
    }

    return ndhead;
err:
    free(ndhead->payload);
    free(ndhead);
    return NULL;
}

int conn_server(struct proxy_dev *pdev) {
    struct netdev_header ndhead;
    int version = NETDEV_PROTOCOL_VERSION;

    ndhead.msgtype = MSGT_CONTROL_VERSION;
    ndhead.size    = sizeof(version);
    ndhead.payload = malloc(ndhead.size);
    memcpy(ndhead.payload, &version, ndhead.size);

    return conn_send(pdev, &ndhead);
}

int conn_client(struct proxy_dev *pdev) {
    struct netdev_header *ndhead;

    ndhead = conn_recv(pdev);

    if (ndhead->msgtype != MSGT_CONTROL_VERSION) {
        printf("conn_client: wrong message type!\n");
        return 0; /* failure */
    }

    

    return 0;
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
        return 0; /* failure */
    }

/* TODO */

    return 1; /* success */
}

/* size is the minimum that should be read */
int recvall(int conn_fd, struct msghdr *msgh, int size) {
    int rvalue = 0, bytes = 0;
    printf("recvall: receiving bytes = %d\n", size);

    do {
        rvalue = recvmsg(conn_fd, msgh, 0);
        fprintf(stderr, "recvmsg: rvalue = %d\n", rvalue);
        if (rvalue == -1) {
            perror("recvall(recvmsg)");
            return 0; /* falure */
        }
        bytes += rvalue;
    } while (bytes < size);

    return 1; /* success */
}

int sendall(int conn_fd, struct msghdr *msgh, int size) {
    int rvalue = 0, bytes = 0;
    printf("recvall: sending bytes = %d\n", size);

    do {
        rvalue = sendmsg(conn_fd, msgh, 0);
        fprintf(stderr, "sendall: rvalue = %d\n", rvalue);
        if (rvalue == -1) {
            perror("recvall(sendmsg)");
            return 0; /* falure */
        }
        bytes += rvalue;
    } while (bytes != size);

    return 1; /* success */
}

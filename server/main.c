#include <string.h> /* memset, memcopy, memmove, memcmp */
#include <unistd.h> /* close */
#include <stdlib.h> /* malloc */
#include <stdio.h>  /* printf */
#include <errno.h>
#include <sys/socket.h>
#include <linux/netlink.h>

#include "protocol.h"

#define MAX_PAYLOAD 1024 /* maximum payload size*/

int netlink_setup() {
    struct sockaddr_nl *p_src_addr;
    int sock_fd = -1;
    int rvalue = 0;

    /* create socket for netlink
     * int socket(int domain, int type, int protocol); */
    sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_PROTOCOL);

    /* check if socket was actually created */
    if ( sock_fd < 0 ) {
        perror("socket error");
        return -1;
    }

    p_src_addr = (struct sockaddr_nl *)malloc(sizeof(*p_src_addr));
    memset(p_src_addr, 0, sizeof(*p_src_addr));
    p_src_addr->nl_family = AF_NETLINK;
    p_src_addr->nl_pid = getpid(); /* self pid */

    /* p_src_addr has to be casted to (struct sockaddr*) for compatibility with
     * IPv4 and IPv6 structures, sockaddr_in and sockaddr_in6. */
    rvalue = bind(sock_fd, (struct sockaddr*)p_src_addr, sizeof(*p_src_addr));

    if ( rvalue < 0 ) {
        perror("socket error");
        return -1;
    }

    /* connect is not needed since we are not using TCP but a raw socket */
    return sock_fd;
}

int main() {
    struct sockaddr_nl dest_addr;
    struct nlmsghdr *p_nlh = NULL;
    struct iovec iov;
    struct msghdr *p_msg; // needs to be global or a pointer
    int sock_fd = -1;
    int rvalue = 0;

    printf("Netlink protocol: %d\n", NETLINK_PROTOCOL);
    sock_fd = netlink_setup();
    printf("sock_fd: %d\n", sock_fd);

    /* zero out struct before using it */
    memset(&dest_addr, 0, sizeof(dest_addr));

    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0; /* For Linux Kernel */
    dest_addr.nl_groups = 0; /* unicast */

    p_nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    memset(p_nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
    p_nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    p_nlh->nlmsg_pid = getpid();
    p_nlh->nlmsg_flags = 0;
    p_nlh->nlmsg_type = MSGTYPE_NETDEV_REGISTER;

    strcpy(NLMSG_DATA(p_nlh), "Register device for sharing");

    iov.iov_base = (void *)p_nlh;
    iov.iov_len = p_nlh->nlmsg_len;
    p_msg = (struct msghdr *)malloc(sizeof(struct msghdr));
    p_msg->msg_name = (void *)&dest_addr;
    p_msg->msg_namelen = sizeof(dest_addr);
    p_msg->msg_iov = &iov;
    p_msg->msg_iovlen = 1;

    printf("Sending message to kernel\n");
    sendmsg(sock_fd,p_msg,0);
    printf("Waiting for message from kernel\n");

    /* Read message from kernel */
    for ( ; ; ) {
        rvalue = recvmsg(sock_fd, p_msg, 0);
        printf("rvalue: %d\n", rvalue);
        if (rvalue <= 0) {
            break;
        }
        printf("Received message payload: %s\n", (char*)NLMSG_DATA(p_nlh));
    }

    close(sock_fd);
}

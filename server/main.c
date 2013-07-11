#include <string.h> /* memset, memcopy, memmove, memcmp */
#include <unistd.h> /* close */
#include <stdlib.h> /* malloc */
#include <stdio.h>  /* printf */
#include <sys/socket.h>
#include <linux/netlink.h>

#include "msgtype.h"

/* defines the protocol used, we want our own protocol */
#define NETLINK_USER 80085

#define MAX_PAYLOAD 1024 /* maximum payload size*/

struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr *nlh = NULL;
struct iovec iov;
struct msghdr msg;
int sock_fd;

int main() {
    /* create socket for netlink
     * int socket(int domain, int type, int protocol); */
    sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_USER);

    /* check if socket was actually created */
    if ( sock_fd < 0 )
        return -1;

    /* zerou out struct before using it */
    memset(&src_addr, 0, sizeof(src_addr));

    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid(); /* self pid */

    /* src_addr has to be casted to (struct sockaddr*) for compatibility with
     * IPv4 and IPv6 structures, sockaddr_in and sockaddr_in6. */
    bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr));

    /* zerou out struct before using it */
    memset(&dest_addr, 0, sizeof(dest_addr));

    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0; /* For Linux Kernel */
    dest_addr.nl_groups = 0; /* unicast */

    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
    nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 0;
    nlh->nlmsg_type = MSGTYPE_NETDEV_REGISTER;

    strcpy(NLMSG_DATA(nlh), "Register device for sharing");

    iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;
    msg.msg_name = (void *)&dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    printf("Sending message to kernel\n");
    sendmsg(sock_fd,&msg,0);
    printf("Waiting for message from kernel\n");

    /* Read message from kernel */
    for ( ; ; ) {
        recvmsg(sock_fd, &msg, 0);
        printf("Received message payload: %s\n", (char*)NLMSG_DATA(nlh));
    }

    close(sock_fd);
}

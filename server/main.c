#include <string.h>        /* memset, memcopy, memmove, memcmp */
#include <unistd.h>        /* close                            */
#include <stdlib.h>        /* malloc                           */
#include <stdio.h>         /* printf                           */
#include <sys/socket.h>    /* socklen_t */
#include <sys/wait.h>      /* waitpid                          */
#include <sys/types.h>     /* pid_t                            */
#include <errno.h>
#include <linux/netlink.h>
#include <arpa/inet.h>      /* sockaddr_in */
#include <netinet/in.h>     /* sockaddr_in */
#include <netinet/ip.h> 
#include <signal.h>         /* signal */

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

void sig_chld(int signo) {
    pid_t pid;
    int stat;

    while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0 ) {
        /* TODO should not use printf in signal handling */
        printf("ALERT: child %d terminated\n", pid);
    }

    return;
}

int netdev_server(int connfd) {
    /* TODO service new connection from a netdev client */
    return 1;
}

int netdev_listener() {
    int listenfd, connfd;
    int rvalue;
    pid_t childpid;
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    if ( listenfd < 0 ) {
        perror("netdev_listener: failed to create socket");
        return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port   = htons(NETDEV_SERVER_PORT);

    rvalue = bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

    if ( rvalue < 0 ) {
        perror("netdev_listener: failed to bind to address");
        return -1;
    }

    rvalue = listen(listenfd, NETDEV_LISTENQ);

    if ( rvalue < 0 ) {
        perror("netdev_listener: failed to listen");
        return -1;
    }

    /* na potrzbey waitpid */
    signal(SIGCHLD, sig_chld);

    while (1) {
        clilen = sizeof(cliaddr);

        if ( (connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen)) < 0 ) {
            if (errno == EINTR) {
                continue;
            } else {
                perror("netdev_listener: accept error");
            }
        }
        
        /* fork for every new client and serve it */
        if ( (childpid = fork()) ==0 ) {
            /* decrease the counter for listening socket, this process won't
             * use it */
            close(listenfd);

            rvalue = netdev_server(connfd);
            if ( rvalue < 0 ) {
                perror("netdev_listener: server process failed");
            }

            exit(0);
        }

        /* decreas the counter for new connection */
        close(connfd);
    }
}

void netdev_client(char *name, char *address) {
    /* TODO start a new connection with a netdev server and register a device
     * with the netdev kernel driver */
    struct sockaddr_nl dest_addr;
    struct nlmsghdr *p_nlh = NULL;
    struct iovec iov;
    struct msghdr *p_msg; // needs to be global or a pointer
    int sock_fd = -1;
    int rvalue = 0;

    printf("Netlink protocol: %d\n", NETLINK_PROTOCOL);
    sock_fd = netlink_setup();
    printf("sock_fd: %d\n", sock_fd);
    if ( sock_fd < 0 ) {
        return;
    }

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
    p_nlh->nlmsg_type = MSGTYPE_CONTROL_ECHO;

    strcpy(NLMSG_DATA(p_nlh), "test echo message");

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
        printf("Received message from pid: %d\
                \n\ttype:\t%d\n\tpayload:\t%s\n",
                p_nlh->nlmsg_pid,
                p_nlh->nlmsg_type,
                (char*)NLMSG_DATA(p_nlh));
    }

    close(sock_fd);
}

int main(int argc, char *argv[]) {
    /* TODO these values will have to be read from a config file */
    netdev_client("netdev","192.168.1.13");

    /* TODO start netdev listener wiating for connections from clients */
    if ( netdev_listener() ) {
        printf("netdev: could not start listener\n");
    }

    return 1;
}

#include <string.h>        /* memset, memcopy, memmove, memcmp */
#include <stdio.h>         /* printf                           */
#include <stdlib.h>        /* malloc                           */
#include <unistd.h>        /* close, getpid                    */
#include <sys/socket.h>    /* socklen_t */
#include <sys/types.h>     /* pid_t                            */
#include <sys/wait.h>      /* waitpid */
#include <arpa/inet.h>      /* inet_notp */
#include <errno.h>
#include <netinet/ip.h>
#include <signal.h>         /* signal */

#include "signal.h"
#include "proxy.h"
#include "protocol.h"

void sig_chld(int signo) {
    pid_t pid;
    int stat;
    printf("sig_chld: received signal\n");

    while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0 ) {
        /* TODO should not use printf in signal handling */
        printf("ALERT: child %d terminated\n", pid);
    }

    return;
}

void sig_hup(int signo) {

    printf("sig_hup: received signal from pid: %d\n", getpid());

    if (netdev_unregister()) {
        printf("sig_hup: failed to send unregister message\n");
    }

    return;
}


void sig_int(int signo) {
    printf("sig_int: received signal from pid: %d\n", getpid());
    return;
}

int netdev_server(int connfd) {
    /* TODO service new connection from a netdev client */
    return 1;
}

void netdev_client(char *name, char *address) {
    /* TODO start a new connection with a netdev server and register
     * a device with the netdev kernel driver */
    struct sockaddr_nl dest_addr;
    struct nlmsghdr *p_nlh = NULL;
    struct iovec iov;
    struct msghdr *p_msg; // needs to be global or a pointer
    int sock_fd = -1;
    int rvalue = 0;
    int msgtype = 0;

    /* send SIGHUP when parent process dies */
    prctl(PR_SET_PDEATHSIG, SIGHUP);
    /* and handle SIGHUP to close the connection */
    signal(SIGHUP, sig_hup);
    signal(SIGINT, sig_int);

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
    p_nlh->nlmsg_type = MSGT_CONTROL_ECHO;

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

    p_nlh->nlmsg_type = MSGT_CONTROL_REGISTER;
    sendmsg(sock_fd,p_msg,0);

    /* Read message from kernel */
    for ( ; ; ) {
        rvalue = recvmsg(sock_fd, p_msg, 0);
        printf("rvalue: %d\n", rvalue);
        if (rvalue <= 0) {
            break;
        }
        msgtype = p_nlh->nlmsg_type;
        
        if (msgtype > MSGT_FO_START && msgtype < MSGT_FO_END ) {
            printf("Received message from pid: %d\
                    \n\ttype:\t%d\n\tpayload:\t%d\n",
                    p_nlh->nlmsg_pid,
                    p_nlh->nlmsg_type,
                    *((int*)NLMSG_DATA(p_nlh)));
        } else {
            printf("Received message from pid: %d\
                    \n\ttype:\t%d\n\tpayload:\t%s\n",
                    p_nlh->nlmsg_pid,
                    p_nlh->nlmsg_type,
                    (char*)NLMSG_DATA(p_nlh));
        }
    }

    close(sock_fd);
}

int netdev_listener() {
    int listenfd, connfd;
    int rvalue;
    pid_t childpid;
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;
    char str[INET_ADDRSTRLEN];
    void *ptr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    if ( listenfd < 0 ) {
        perror("netdev_listener: failed to create socket");
        return -1;
    }
    printf("netdev_listener: listenfd = %d\n", listenfd);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port   = htons(NETDEV_SERVER_PORT);

    rvalue = bind(listenfd,
                (SA *)&servaddr,
                sizeof(servaddr));

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

    printf("netdev_listener: starting listener\n");
    while (1) {
        clilen = sizeof(cliaddr);

        if ((connfd = accept(listenfd, (SA *) &cliaddr, &clilen)) < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                perror("netdev_listener: accept error");
            }
        }

        ptr = (char *)inet_ntop(cliaddr.sin_family ,
                                &cliaddr.sin_addr,
                                str,
                                sizeof(str));

        if ( ptr != NULL ) {
            printf("netdev_listener: new connection from %s:%d\n",
                    str,
                    ntohs(cliaddr.sin_port));
        }

        /* fork for every new client and serve it */
        if ( (childpid = fork()) ==0 ) {
            /* decrease the counter for listening socket,
             * this process won't use it */
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

int main(int argc, char *argv[]) {
    int pid;

    /* TODO these values will have to be read from a config file */
    if ( ( pid = fork() ) == 0 ) {
        netdev_client("netdev","192.168.1.13");
        exit(0);
    } else if ( pid < 0 ) {
        perror("main: failed to fork");
    }

    /* TODO start netdev listener wiating for connections from clients */
    if ( netdev_listener() ) {
        printf("netdev: could not start listener\n");
    }

    return 1;
}

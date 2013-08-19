#include <stdlib.h>        /* malloc                           */
#include <stdio.h>         /* printf                           */
#include <unistd.h>        /* close, getpid                    */
#include <sys/prctl.h>      /* prctl */
#include <signal.h>         /* signal */
#include <errno.h>

#include "proxy.h"
#include "protocol.h"
#include "signal.h"
#include "helper.h"
#include "netlink.h"
#include "conn.h"
#include "debug.h"

void proxy_setup_signals()
{
    /* send SIGHUP when parent process dies */
    prctl(PR_SET_PDEATHSIG, SIGHUP);
    /* and handle SIGHUP to safely unregister the device */
    signal(SIGHUP, proxy_sig_hup);
    /* ignore INT since it comes before HUP */
    signal(SIGINT, SIG_IGN);
}

int proxy_setup_unixsoc(struct proxy_dev *pdev)
{
    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, pdev->us_fd) == -1) {
        perror("proxy_setup_unixsoc");
        return -1; /* failure */
    }
    return 0; /* success */
}

struct proxy_dev * proxy_alloc_dev(int connfd)
{
    struct proxy_dev *pdev = NULL;
    socklen_t addrlen = 0;

    pdev = malloc(sizeof(struct proxy_dev));

    if (!pdev) {
        perror("proxy_alloc_dev(malloc)");
        return NULL;
    }

    pdev->rm_addr = malloc(sizeof(*pdev->rm_addr));
    addrlen = sizeof(*pdev->rm_addr);

    if (getpeername(connfd, (struct sockaddr *)pdev->rm_addr, &addrlen) == -1) {
        perror("proxy_alloc_dev(getperrname)");
        return NULL;
    }

    pdev->rm_fd = connfd;
    pdev->pid   = getpid();
    pdev->client = false; /* this is a server instance */

    return pdev;
}

void proxy_destroy(struct proxy_dev *pdev) {
    free(pdev->nl_src_addr);
    free(pdev->nl_dst_addr);
    free(pdev->rm_addr);
    free(pdev->us_addr);
    free(pdev->rm_ipaddr);
    free(pdev->remote_dev_name);
    free(pdev->dummy_dev_name);
    free(pdev);
}

void proxy_client(struct proxy_dev *pdev)
{
    pdev->pid = getpid(); /* make sure you have the right pid */
    proxy_setup_signals();
    printf("proxy_client: starting connection to server, pid = %d\n", pdev->pid);

    if (proxy_setup_unixsoc(pdev) == -1) {
        printf("proxy_client: failed to setup control socket\n");
        goto err;
    }

    /* setup a netlink connection with the kernel driver, rest is
     * pointless if we can't connect with the kernel driver */
    if (netlink_setup(pdev) == -1) {
        printf("proxy_client: failed to setup netlink socket!\n");
        goto err;
    }

    /* connect to the server */
    if (conn_server(pdev) == -1) {
        printf("proxy_client: failed to connect to the server!\n");
        goto err;
    }

    /* send a device registration request to the server */
    if (conn_send_dev_reg(pdev) == -1) {
        printf("proxy_client: failed to register device on the server!\n");
        goto err;
    }

    /* start loop that will forward all file operations */
    if (proxy_loop(pdev) == -1) {
        printf("proxy_client: loop broken\n");
    }

    if (!netlink_unregister_dev(pdev)) {
        printf("proxy_client: failed to unregister device!\n");
    }

err:
    proxy_destroy(pdev);
    return;
}

void proxy_server(int connfd)
{
    struct proxy_dev *pdev = NULL;

    proxy_setup_signals();

    if ((pdev = proxy_alloc_dev(connfd)) == NULL) {
        printf("proxy_server: failed to allocate proxy_dev\n");
        return;
    }
    printf("proxy_server: starting serving client, pid = %d\n", pdev->pid);

    if (proxy_setup_unixsoc(pdev) == -1) {
        printf("proxy_client: failed to setup control socket\n");
        return;
    }

    /* setup a netlink connection with the kernel driver, rest is pointless
     * if we can't connect to the kernel driver */
    if (netlink_setup(pdev) == -1) {
        printf("proxy_server: failed to setup netlink socket!\n");
        return;
    }

    /* connect to the client */
    if (conn_client(pdev) == -1) {
        printf("proxy_server: failed to connect to the server!\n");
        return;
    }

    /* receive a device registration request */
    if (conn_recv_dev_reg(pdev) == -1) {
        printf("proxy_server: failed to register device on the server!\n");
        return;
    }

    /* start loop that will forward all file operations */
    if (proxy_loop(pdev) == -1) {
        printf("proxy_server: loop broken\n");
    }

    if (netlink_unregister_dev(pdev) == -1) {
        printf("proxy_server: failed to unregister device!\n");
    }

    proxy_destroy(pdev);
    /* close the socket completely */
    close(connfd);
    return;
}

int proxy_loop(struct proxy_dev *pdev)
{
    int maxfd, nready;
    fd_set rset;

    debug("starting serving connections");

    /* Read message from kernel */
    for ( ; ; ) {
        FD_ZERO(&rset); /* clear all file descriptors */
        FD_SET(pdev->us_fd[0], &rset); /* add unix socket */
        FD_SET(pdev->nl_fd, &rset); /* add netlink socket */
        FD_SET(pdev->rm_fd, &rset); /* add remote socket */
        maxfd = max(pdev->nl_fd, pdev->rm_fd);
        maxfd = max(maxfd, pdev->us_fd[0]);
        maxfd++; /* this is a count of how far the sockets go */

        if ((nready = select(maxfd, /* max number of file descriptors */
                            &rset,  /* read file descriptors */
                            NULL,   /* no write fd */
                            NULL,   /* no exception fd */
                            NULL)   /* no timeout */
                            ) == -1 ) {
            perror("proxy_loop(select)");
            debug("errno = %d", errno);
            return -1; /* failure */
        }

        if (FD_ISSET(pdev->us_fd[0], &rset)) {
            /* message from control unix socket */
            if (proxy_control(pdev) == -1) {
                return -1;
            }
            break;
        }

        if (FD_ISSET(pdev->nl_fd, &rset)) {
            if (proxy_handle_netlink(pdev) == -1) {
                return -1;
            }
        }

        if (FD_ISSET(pdev->rm_fd, &rset)) {
            if (proxy_handle_remote(pdev) == -1) {
                return -1;
            }
        }
    }

    return 0; /* success */
}

int proxy_control(struct proxy_dev *pdev)
{
    printf("proxy_control: not yet implemented\n");
    return -1;
}

int proxy_handle_remote(struct proxy_dev *pdev)
{
    struct netdev_header *ndhead = NULL;
    int rvalue , error;
    socklen_t len;

    rvalue = getsockopt(pdev->rm_fd, SOL_SOCKET, SO_ERROR, &error, &len);
    if (rvalue) {
        printf("proxy_handle_remote: connection lost\n");
        return -1;
    }

    ndhead = conn_recv(pdev);
    if (!ndhead) {
        printf("proxy_handle_remote: failed to receive message\n");
        return -1;
    }


    if (ndhead->msgtype > MSGT_FO_START &&
        ndhead->msgtype < MSGT_FO_END) {
        /*printf("proxy_handle_remote:  FILE OPERATION: %d\n",
                ndhead->msgtype);
        */

        netlink_send_nlh(pdev, (struct nlmsghdr *)ndhead->payload);
    } else {
        printf("proxy_handle_remote: unknown message type: %d\n",
                ndhead->msgtype);
        return -1; /* failure */
    }

    return 0;
}

/* at the moment the only thing the kernel will send are file
 * operations so I will ignore other possibilities */
int proxy_handle_netlink(struct proxy_dev *pdev)
{
    struct nlmsghdr *nlh = NULL;
    struct netdev_header *ndhead = NULL;

    nlh = netlink_recv(pdev);

    if (!nlh) {
        printf("proxy_handle_netlink: failed to receive message\n");
        return -1; /* failure */
    }

    if (nlh->nlmsg_type > MSGT_FO_START &&
        nlh->nlmsg_type < MSGT_FO_END) {
        /*printf("proxy_handle_netlink: FILE OPERATION: %d\n",
                nlh->nlmsg_type);
        */

        ndhead = malloc(sizeof(*ndhead));
        if (!ndhead) {
            perror("proxy_handle_netlink(malloc)");
            return -1;
        }
        ndhead->msgtype = nlh->nlmsg_type;
        ndhead->size = nlh->nlmsg_len;
        ndhead->payload = nlh;

        conn_send(pdev, ndhead);
    } else {
        printf("proxy_handle_netlink: unknown message type: %d\n",
                nlh->nlmsg_type);
        return -1; /* failure */
    }

    /* if error or this message says it wants a response */

    return 0; /* success */
}

void proxy_sig_hup(int signo)
{

    printf("sig_hup: received signal in process: %d\n", getpid());

    /*
    if (proxy_unregister_device()) {
        printf("sig_hup: failed to send unregister message\n");
    }
    */

    return;
}

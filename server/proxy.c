#include <stdlib.h>        /* malloc                           */
#include <stdio.h>         /* printf                           */
#include <unistd.h>        /* close, getpid                    */
#include <sys/prctl.h>      /* prctl */
#include <signal.h>         /* signal */

#include "proxy.h"
#include "protocol.h"
#include "signal.h"
#include "helper.h"
#include "netlink.h"
#include "conn.h"

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
        return 0; /* failure */
    }
    return 1; /* success */
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

    if (getpeername(connfd, (SA *)pdev->rm_addr, &addrlen) == -1) {
        perror("proxy_alloc_dev(getperrname)");
        return NULL;
    }

    pdev->rm_fd = connfd;
    pdev->pid   = getpid();
    //pdev->us_fd = malloc(2 * sizeof(int)); /* space for unix file descriptors */

    return pdev;
}

void proxy_client(struct proxy_dev *pdev)
{
    pdev->pid = getpid(); /* make sure you have the right pid */
    proxy_setup_signals();
    printf("proxy_client: starting connection to server, pid = %d\n", pdev->pid);

    if (!proxy_setup_unixsoc(pdev)) {
        printf("proxy_client: failed to setup control socket\n");
        return;
    }

    /* setup a netlink connection with the kernel driver, rest is
     * pointless if we can't connect with the kernel driver */
    if (!netlink_setup(pdev)) {
        printf("proxy_client: failed to setup netlink socket!\n");
        return;
    }

    /* connect to the server */
    /*
    if (!conn_server(pdev)) {
        printf("proxy_client: failed to connect to the server!\n");
        return;
    }
    */

    /* send a device registration request to the server */
    /*
    if (!conn_send_dev_reg(pdev)) {
        printf("proxy_client: failed to register device on the server!\n");
        return;
    }
    */

    /* register a dummy device with the kernel */
    if (!netlink_reg_dummy_dev(pdev)) {
        printf("proxy_client: failed to register device on the server!\n");
        return;
    }

    /* start loop that will forward all file operations */
    if (!proxy_loop(pdev)) {
        printf("proxy_client: loop broken\n");
    }

    if (!netlink_unregister_dev(pdev)) {
        printf("proxy_client: failed to unregister device!\n");
    }

    /* TODO free proxy_dev */
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

    if (!proxy_setup_unixsoc(pdev)) {
        printf("proxy_client: failed to setup control socket\n");
        return;
    }

    /* setup a netlink connection with the kernel driver, rest is pointless
     * if we can't connect to the kernel driver */
    /*
    if (!netlink_setup(pdev)) {
        printf("proxy_server: failed to setup netlink socket!\n");
        return;
    }
    */

    /* connect to the client */
    if (!conn_client(pdev)) {
        printf("proxy_server: failed to connect to the server!\n");
        return;
    }

    /* receive a device registration request */
    if (!conn_recv_dev_reg(pdev)) {
        printf("proxy_server: failed to register device on the server!\n");
        return;
    }

    /* register a dummy device with the kernel */
    /*
    if (!netlink_reg_remote_dev(pdev)) {
        printf("proxy_server: failed to register device on the server!\n");
        return;
    }
    */

    /* start loop that will forward all file operations */
    if (!proxy_loop(pdev)) {
        printf("proxy_server: loop broken\n");
    }

    if (!netlink_unregister_dev(pdev)) {
        printf("proxy_server: failed to unregister device!\n");
    }

    /* TODO free proxy_data */
    return;
}

int proxy_loop(struct proxy_dev *pdev)
{
    int maxfd, nready;
    fd_set rset;

    /* Read message from kernel */
    for ( ; ; ) {
        FD_ZERO(&rset); /* clear all file descriptors */
        FD_SET(pdev->us_fd[0], &rset); /* add unix socket */
        FD_SET(pdev->nl_fd, &rset); /* add netlink socket */
        //FD_SET(pdev->rm_fd, &rset); /* add remote socket */
        maxfd = max(pdev->nl_fd, pdev->rm_fd);
        maxfd = max(maxfd, pdev->us_fd[0]);
        maxfd++; /* this is a count of how far the sockets go */

        if ((nready = select(maxfd, /* max number of file descriptors */
                            &rset,   /* read file descriptors */
                            NULL,   /* no write fd */
                            NULL,   /* no exception fd */
                            NULL)   /* no timeout */
                            ) == -1 ) {
            perror("proxy_loop(select)");
            return 0; /* failure */
        }
        printf("proxy_loop: one of data sources is ready!\n");

        if (FD_ISSET(pdev->us_fd[0], &rset)) {
            printf("proxy_loop: unix socket data!\n");
            /* message from control unix socket */
            if (!proxy_control(pdev)) {
                break;
            }
            break;
        }

        if (FD_ISSET(pdev->nl_fd, &rset)) {
            /* message from kernel, send to remote */
            proxy_handle_netlink(pdev);
        }

        if (FD_ISSET(pdev->rm_fd, &rset)) {
            /* message from remote, send to kernel */
            proxy_handle_remote(pdev);
        }
    }

    return 1; /* success */
}

int proxy_control(struct proxy_dev *pdev)
{
    return 1;
}

int proxy_handle_remote(struct proxy_dev *pdev)
{
    return 1;
}

/* at the moment the only thing the kernel will send are file
 * operations so I will ignore other possibilities */
int proxy_handle_netlink(struct proxy_dev *pdev)
{
    struct nlmsghdr *nlh = NULL;

    nlh = netlink_recv(pdev);

    if (!nlh) {
        printf("proxy_handle_netlink: failed to receive message\n");
        return 0; /* failure */
    }

    if (nlh->nlmsg_type > MSGT_FO_START && nlh->nlmsg_type < MSGT_FO_END ) {
        /* TODO send to server */
        netlink_send(pdev, nlh);
    } else {
        printf("proxy_handle_netlink: unknown message type: %d\n",
                nlh->nlmsg_type);
        return 0; /* failure */
    }

    /* if error or this message says it wants a response */

    return 1; /* success */
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

#include <stdio.h>
#include <stdlib.h>

#include "netlink.h"
#include "protocol.h"
#include "netprotocol.h"
int netlink_setup(
    struct proxy_dev *pdev)
{
    int rvalue = 0;

    /* create socket for netlink */
    pdev->nl_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_PROTOCOL);

    /* check if socket was actually created */
    if ( pdev->nl_fd < 0 ) {
        perror("netlink_setup(socket setup)");
        return 0;
    }

    /* allocate memory fo addreses and zero them out */
    pdev->nl_src_addr = malloc(sizeof(*pdev->nl_src_addr));
    pdev->nl_dst_addr = malloc(sizeof(*pdev->nl_dst_addr));
    pdev->msgh=         malloc(sizeof(*pdev->msgh));
    pdev->nlh =         malloc(NLMSG_SPACE(MAX_PAYLOAD)); /* TODO */
    pdev->iov =         malloc(NLMSG_SPACE(MAX_PAYLOAD)); /* TODO */
    memset(pdev->nl_src_addr, 0, sizeof(*pdev->nl_src_addr));
    memset(pdev->nl_dst_addr, 0, sizeof(*pdev->nl_dst_addr));
    memset(pdev->msgh, 0, sizeof(*pdev->msgh));
    memset(pdev->nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));

    /* set up source address: this process, for bind */
    pdev->nl_src_addr->nl_family = AF_NETLINK;
    pdev->nl_src_addr->nl_pid = pdev->pid; /* self pid */
    pdev->nl_src_addr->nl_groups = 0; /* unicast */

    /* set up destination address: kernel, for later */
    pdev->nl_dst_addr->nl_family = AF_NETLINK;
    pdev->nl_dst_addr->nl_pid = 0; /* For Linux Kernel */
    pdev->nl_dst_addr->nl_groups = 0; /* unicast */

    rvalue = bind(pdev->nl_fd,
                (SA *)pdev->nl_src_addr,
                sizeof(*pdev->nl_src_addr));

    if ( rvalue < 0 ) {
        perror("netlink_setup(binding socket)");
        return 0; /* failure */
    }

    printf("netlink_setup: complete\n");
    /* connect is not needed since we are not using TCP but a raw socket */
    return 1; /* success */
}

int netlink_send(
    struct proxy_dev *pdev,
    void *buff,
    size_t bsize,
    int msgtype,
    int flags)
{
    struct nlmsgerr *msgerr = NULL;

    printf("netlink_send: sending message to kernel\n");
    printf("netlink_send: buff = %s, bsize = %zu\n", (char*)buff, bsize);

    pdev->nlh->nlmsg_len = NLMSG_SPACE(bsize); /* struct size + payload */
    pdev->nlh->nlmsg_pid = pdev->pid;
    printf("netlink_send: nlh->nlmsg_pid = %d\n", pdev->nlh->nlmsg_pid);
    pdev->nlh->nlmsg_type = msgtype;
    /* delivery confirmation with NLMSG_ERROR and error field set to 0 */
    pdev->nlh->nlmsg_flags = flags | NLM_F_ACK;

    /* TODO more data about device */
    if (buff) {
        memcpy(NLMSG_DATA(pdev->nlh), buff, bsize);
    }

    /* netlink header is our payload */
    pdev->iov->iov_base = (void *)pdev->nlh;
    pdev->iov->iov_len = pdev->nlh->nlmsg_len;

    pdev->msgh->msg_name = (void *)pdev->nl_dst_addr;
    pdev->msgh->msg_namelen = sizeof(*pdev->nl_dst_addr);
    pdev->msgh->msg_iov = pdev->iov; /* this normally is an array of */
    pdev->msgh->msg_iovlen = 1; /* TODO, we won't always use just one */

    /* send everything */
    if ( sendmsg(pdev->nl_fd, pdev->msgh, 0) == -1 ) {
        perror("netlink_send(sendmsg)");
        return 0; /* failure */
    }

    /* get confirmation of delivery */
    if ( recvmsg(pdev->nl_fd, pdev->msgh, 0) == -1 ) {
        perror("netlink_send(recvmsg)");
        return 0; /* failure */
    } else {
        if ( pdev->nlh->nlmsg_type == NLMSG_ERROR ) {
            printf("netlink_send: nlmsgerr size = %d\n",
                    pdev->nlh->nlmsg_len);
            msgerr = ((struct nlmsgerr*)NLMSG_DATA(pdev->nlh));
            if (msgerr->error != 0) {
                printf("netlink_send: delivery failure!\n");
                printf("netlink_send: msgerr->error = %d\n",
                        ntohl(msgerr->error));
                return 0; /* failure */
            } else {
                printf("netlink_send: delivery success!\n");
            }
        } else {
            printf("netlink_send: next message was not confirmation!\n");
            return 0; /* failure */
        }
    }

    return 1; /* success */
}

struct netlink_message * netlink_recv(struct proxy_dev *pdev) {
    struct netlink_message *nlmsg = NULL;
    int bytes;

    bytes = recvmsg(pdev->nl_fd, pdev->msgh, 0);

    if ( bytes <= -1 ) {
        perror("netlink_recv(recvmsg)");
        return NULL;
    }

    nlmsg = malloc(sizeof(*nlmsg));
    if (!nlmsg) {
        perror("netlink_recv(malloc)");
    }

    nlmsg->msgtype = pdev->nlh->nlmsg_type;
    nlmsg->size = pdev->nlh->nlmsg_len;
    nlmsg->payload = NLMSG_DATA(pdev->nlh);

    printf("netlink_recv: msgtype = %d\n\tsize = %zu\n",
            nlmsg->msgtype,
            nlmsg->size);

    return nlmsg;
}

int netlink_reg_dummy_dev(
    struct proxy_dev *pdev)
{
    printf("netlink_reg_dummy_dev: sending message to kernel\n");
    netlink_send(pdev, pdev->dummy_dev_name, sizeof(pdev->dummy_dev_name), MSGT_CONTROL_REG_DUMMY, NLM_F_REQUEST);
    printf("netlink_reg_dummy_dev: waiting for message from kernel\n");
    
    return 1; /* success */
}

int netlink_reg_remote_dev(
    struct proxy_dev *pdev)
{
    return 0;
}

int netlink_unregister_dev(
    struct proxy_dev *pdev)
{
    netlink_send(pdev, 0, 0, MSGT_CONTROL_UNREGISTER, NLM_F_REQUEST);
    return 0;
}

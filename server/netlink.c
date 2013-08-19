#include <stdio.h> /* perror */
#include <stdlib.h> /* malloc */

#include "netlink.h"
#include "protocol.h"
#include "netprotocol.h"
#include "conn.h"
#include "debug.h"

int netlink_setup(
    struct proxy_dev *pdev)
{
    int rvalue = 0;

    /* create socket for netlink */
    pdev->nl_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_PROTOCOL);

    /* check if socket was actually created */
    if (pdev->nl_fd < 0) {
        perror("netlink_setup(socket setup)");
        return -1;
    }

    /* allocate memory fo addreses and zero them out */
    pdev->nl_src_addr = malloc(sizeof(*pdev->nl_src_addr));
    pdev->nl_dst_addr = malloc(sizeof(*pdev->nl_dst_addr));
    memset(pdev->nl_src_addr, 0, sizeof(*pdev->nl_src_addr));
    memset(pdev->nl_dst_addr, 0, sizeof(*pdev->nl_dst_addr));

    /* set up source address: this process, for bind */
    pdev->nl_src_addr->nl_family = AF_NETLINK;
    pdev->nl_src_addr->nl_pid = pdev->pid; /* self pid */
    pdev->nl_src_addr->nl_groups = 0; /* unicast */

    /* set up destination address: kernel, for later */
    pdev->nl_dst_addr->nl_family = AF_NETLINK;
    pdev->nl_dst_addr->nl_pid = 0; /* For Linux Kernel */
    pdev->nl_dst_addr->nl_groups = 0; /* unicast */

    rvalue = bind(pdev->nl_fd,
                (struct sockaddr*)pdev->nl_src_addr,
                sizeof(*pdev->nl_src_addr));

    if (rvalue < 0) {
        perror("netlink_setup(binding socket)");
        return -1; /* failure */
    }

    printf("netlink_setup: complete\n");
    /* connect is not needed since we are not using TCP but a raw socket */
    return 0; /* success */
}

int netlink_send(
    struct proxy_dev *pdev,
    struct nlmsghdr *nlh)
{
    struct msghdr msgh = {0};
    struct iovec iov = {0};

    if (!nlh) {
        printf("netlink_send: nlh is NULL\n");
        return -1; /* failure */
    }

    /*
    debug("type: %d, pid: %d, size: %d",
            nlh->nlmsg_type, nlh->nlmsg_pid, nlh->nlmsg_len);
    */

    /* netlink header is our payload */
    iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;

    msgh.msg_iov = &iov; /* this normally is an array of */
    msgh.msg_iovlen = 1; /* TODO, we won't always use just one */

    if (sendall(pdev->nl_fd, &msgh, nlh->nlmsg_len) == -1) {
        printf("netlink_send: failed to send message\n");
    }

    free(nlh);
    return 0; /* success */
}

int netlink_send_msg(
    struct proxy_dev *pdev,
    void *buff,
    size_t bsize,
    int msgtype,
    int flags)
{
    struct nlmsghdr *nlh = NULL;

    nlh = malloc(NLMSG_SPACE(bsize));
    if (!nlh) {
        printf("netlink_send_msg: failed to allocate nlh\n");
        return -1; /* failure */
    }
    memset(nlh, 0, NLMSG_SPACE(bsize));

    nlh->nlmsg_len = NLMSG_SPACE(bsize); /* struct size + payload */
    nlh->nlmsg_pid = pdev->pid;
    nlh->nlmsg_type = msgtype;
    /* delivery confirmation with NLMSG_ERROR and error field set to 0 */
    nlh->nlmsg_flags = flags | NLM_F_ACK;

    /* TODO check if we can't just assign the pointer */
    if (buff) {
        memcpy(NLMSG_DATA(nlh), buff, bsize);
    }

    if (netlink_send_nlh(pdev, nlh) == -1) {
        printf("netlink_send_msg: failed to send message\n");
        return -1; /* failure */
    }

    return 0; /* success */
}

int netlink_send_nlh(
    struct proxy_dev *pdev,
    struct nlmsghdr *nlh)
{
    struct nlmsgerr *msgerr = NULL;
    nlh->nlmsg_pid = pdev->pid;

    /* send everything */
    if (netlink_send(pdev, nlh) == -1) {
        printf("netlink_send_nlh: failed to send message\n");
        return -1; /* failure */
    }
    /* get confirmation of delivery */
    if ((nlh = netlink_recv(pdev)) == NULL) {
        printf("netlink_send_nlh: failed to receive confirmation\n");
        return -1; /* failure */
    }

    /* check confirmation */
    if ( nlh->nlmsg_type == NLMSG_ERROR ) {
        msgerr = ((struct nlmsgerr*)NLMSG_DATA(nlh));
        if (msgerr->error != 0) {
            debug("delivery failure, msgerr->error = %d", msgerr->error);
            return -1; /* failure */
        }
    } else {
        printf("netlink_send: next message was not confirmation!\n");
        return -1; /* failure */
    }

    return 0; /* success */
}

struct nlmsghdr * netlink_recv(
    struct proxy_dev *pdev)
{
    struct nlmsghdr *nlh = NULL;
    struct msghdr msgh = {0};
    struct iovec iov = {0};
    size_t bufflen = sizeof(*nlh);
    void *buffer = malloc(bufflen);

    /* netlink header is our payload */
    iov.iov_base = buffer;
    iov.iov_len = bufflen;
    msgh.msg_iov = &iov; /* this normally is an array of */
    msgh.msg_iovlen = 1;
    msgh.msg_flags = MSG_PEEK; /* first we need the full msg size */

    /* the minimum is the size of the nlmsghdr alone */
    bufflen = recvall(pdev->nl_fd, &msgh, bufflen);
    if (bufflen == -1) {
        printf("netlink_recv: failed to read message\n");
        goto err;
    }

    nlh = buffer;
    /*
    debug("type: %d, pid: %d, size: %d",
            nlh->nlmsg_type, nlh->nlmsg_pid, nlh->nlmsg_len);
    */

    /* increase the buffer size if needbe */
    bufflen = nlh->nlmsg_len;
    buffer = realloc(buffer, bufflen);
    iov.iov_base = buffer;
    iov.iov_len = bufflen;
    msgh.msg_flags = 0; /* get rid of MSG_PEEK */

    /* get the rest of message */
    bufflen = recvall(pdev->nl_fd, &msgh, bufflen);
    if (bufflen == -1) {
        printf("netlink_recv: failed to read message\n");
        goto err;
    }

    nlh = buffer;
    return nlh;
err:
    free(buffer);
    return NULL;
}

int netlink_reg_dummy_dev(
    struct proxy_dev *pdev)
{
    return netlink_send_msg(pdev,
                        pdev->dummy_dev_name,
                        strlen(pdev->dummy_dev_name)+1,
                        MSGT_CONTROL_REG_DUMMY,
                        NLM_F_REQUEST);
}

int netlink_reg_remote_dev(
    struct proxy_dev *pdev)
{
    return netlink_send_msg(pdev,
                        pdev->remote_dev_name,
                        strlen(pdev->remote_dev_name)+1,
                        MSGT_CONTROL_REG_SERVER,
                        NLM_F_REQUEST);
}

int netlink_unregister_dev(
    struct proxy_dev *pdev)
{
    return netlink_send_msg(pdev,
                        NULL, /* no payload */
                        0, /* 0 payload size */
                        MSGT_CONTROL_UNREGISTER,
                        NLM_F_REQUEST);
}

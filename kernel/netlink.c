#include <linux/netlink.h>  /* for netlink sockets */
#include <linux/module.h>
#include <net/sock.h>

#include "netlink.h"
#include "netdevmgm.h"
#include "protocol.h"
#include "dbg.h"

struct sock *nl_sk = NULL;

void netlink_recv(struct sk_buff *skb)
{
    struct nlmsghdr *nlh;
    int pid, msgtype, seq, err = 0;

    nlh = nlmsg_hdr(skb);

    pid     = nlh->nlmsg_pid;
    seq     = nlh->nlmsg_seq;
    msgtype = nlh->nlmsg_type;

    debug("msg type: %d, from pid: %d, msg size: %d",
            msgtype, pid, nlh->nlmsg_len);
    if ( pid == 0 ) {
        printk(KERN_ERR "netlink_recv: received message from kernel, pid: %d\n", pid);
        return;
    }

    /* if msgtype is a netdev control message */
    if (msgtype >= MSGT_CONTROL_START && msgtype < MSGT_CONTROL_END) {
        switch (msgtype) {
        case MSGT_CONTROL_ECHO:
            err = netlink_echo(pid, seq, (char*)nlmsg_data(nlh));
            break;
        case MSGT_CONTROL_VERSION:
            break;
        case MSGT_CONTROL_REG_DUMMY:
            err = ndmgm_create_dummy(pid, (char*)nlmsg_data(nlh));
            break;
        case MSGT_CONTROL_REG_SERVER:
            err = ndmgm_create_server(pid, (char*)nlmsg_data(nlh));
            break;
        case MSGT_CONTROL_UNREGISTER:
            err = ndmgm_find_destroy(pid);
            break;
        case MSGT_CONTROL_RECOVER:
            break;
        case MSGT_CONTROL_DRIVER_STOP:
            break;
        case MSGT_CONTROL_LOSTCONN:
            break;
        default:
            printk(KERN_ERR
                    "netlink_recv: unknown control msg type: %d\n",
                    msgtype);
            break;
        }
    /* if it's not a control message it has to be a file operation */
    } else if ( msgtype > MSGT_FO_START && msgtype < MSGT_FO_END ) {
        debug("file opration message");
        err = fo_recv(skb);
    /* otherwise we don't know what this message type is */
    } else {
        debug("unknown message type: %d",
                            msgtype);
    }

    /* if error or if this message says it wants a response */
    if ( err || (nlh->nlmsg_flags & NLM_F_ACK )) {
        debug("sending ACK, err = %d", err);
        netlink_ack(skb, nlh, err); /* err should be 0 when no error */
    } else {
        /* if we don't send ack we have to free sk_buff */
        debug("not sending ACK");
        kfree_skb(skb);
    }
}

int netlink_send(
    struct netdev_data *nddata,
    int seq,
    short msgtype,
    int flags,
    void *buff,
    size_t bufflen)
{
    struct nlmsghdr *nlh;
    struct sk_buff *skb;
    int rvalue;

    debug("msg type: %d, to pid: %d, msg size: %zu",
            msgtype, nddata->nlpid, bufflen);

    /* allocate space for message header and it's payload */
    skb = nlmsg_new(bufflen, GFP_KERNEL);

    if(!skb) {
        printk(KERN_ERR "netlink_send: failed to allocate new sk_buff!\n");
        goto free_skb;
    }

    /* set pid, seq, message type, payload size and flags */
    nlh = nlmsg_put(
                    skb,
                    nddata->nlpid,
                    seq,
                    msgtype,
                    bufflen, /* adds nlh size (16bytes) for full size */
                    flags | NLM_F_ACK); /* for delivery confirmation */

    if (!nlh) {
        printk(KERN_ERR "netlink_send: failed to create nlh\n");
        goto free_skb;
    }

    debug("nlh->nlmsg_len = %d", nlh->nlmsg_len);

    NETLINK_CB(skb).dst_group = 0; /* not in mcast group */

    if (buff) { /* possible to send message without payload */
        memcpy(nlmsg_data(nlh) ,buff ,bufflen);
    }

    /* send messsage,nlmsg_unicast will take care of freeing skb */
    rvalue = nlmsg_unicast(nl_sk, skb, nddata->nlpid);

    /* TODO get confirmation of delivery */

    if(rvalue > 0) {
        printk(KERN_INFO "netlink_send: error while sending back to user\n");
        return 0; /* failure */
    }

    return 1; /* success */
free_skb:
    nlmsg_free(skb_out);
    return 0;
}

int netlink_echo(int pid, int seq, char *msg)
{
    struct nlmsghdr *nlh;
    struct sk_buff *skb;
    int msg_size = strlen(msg);
    int rvalue;

    printk(KERN_INFO "netlink: received msg payload: %s\n", msg);

    /* allocate memory for sk_buff, no flags */
    skb = nlmsg_new(msg_size, GFP_KERNEL);

    if(!skb) {
        printk(KERN_ERR "netlink: failed to allocate new sk_buff!\n");
        rvalue = -1;
        goto skbfree;
    }

    /* set pid, seq, message type, payload size and flags */
    nlh = nlmsg_put(
                    skb,
                    pid,
                    seq,
                    MSGT_CONTROL_ECHO,
                    msg_size,
                    NLM_F_ACK);

    NETLINK_CB(skb).dst_group = 0; /* not in mcast group */

    strncpy(nlmsg_data(nlh) ,msg ,msg_size);

    /* send messsage, nlmsg_unicast will take care of freeing skb */
    rvalue = nlmsg_unicast(nl_sk,skb,pid);

    if(rvalue<=0) {
        printk(KERN_ERR "netlink: error while replying to process\n");
        return -1;
    }

    return 0; /* success */
skbfree:
    nlmsg_free(skb);
    return -1;
}

int netlink_init(void)
{
    /* NOTE: new structure for configuring netlink socket changed in
     * linux kernel 3.8
     * http://stackoverflow.com/questions/15937395/netlink-kernel-create-is-not-working-with-latest-linux-kernel
     */
    struct netlink_kernel_cfg nl_cfg;
    nl_cfg.groups = 0;           // used for multicast
    nl_cfg.flags = 0;            // TODO
    nl_cfg.input = netlink_recv; // function that will receive data

    printk("netdev: Netlink protocol: %d\n", NETLINK_PROTOCOL);

    nl_sk = netlink_kernel_create(&init_net,
                                    NETLINK_PROTOCOL,
                                    &nl_cfg);

    if(!nl_sk) {
        printk(KERN_ALERT "Error creating socket.\n");
        return -1;
    }

    printk("netdev: Netlink setup complete\n");
    return 0;
}

void netlink_exit(void)
{
    printk(KERN_INFO "netdev: closing Netlink\n");
    /* TODO will have to close connections of all devices with the server
     * process or just close the socket if each device will have it's own
     * process/socket */

    /* can only be done once the process releases the socket, otherwise
     * kernel will refuse to unload the module */
    netlink_kernel_release(nl_sk);
}

MODULE_LICENSE("GPL");

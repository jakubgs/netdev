#include <linux/netlink.h>  /* for netlink sockets */
#include <linux/module.h>
#include <net/sock.h>
#include <linux/kthread.h> /* task_struct, kthread_run */
#include <linux/ktime.h> /* getnstimeofday */

#include "netlink.h"
#include "netdevmgm.h"
#include "protocol.h"
#include "dbg.h"

struct sock *nl_sk = NULL;

void prnttime(void) {
    struct timespec ts;

    getnstimeofday(&ts);
    
    printk(KERN_DEBUG "%pS - TIME: sec %ld, nsec %ld\n",
            __builtin_return_address(0),
            ts.tv_sec, ts.tv_nsec);
}

void netlink_recv(struct sk_buff *skb)
{
    struct nlmsghdr *nlh;
    struct task_struct *task = NULL;
    int pid, msgtype, seq, err = 0;
    void *data;

    nlh = nlmsg_hdr(skb);

    pid     = nlh->nlmsg_pid;
    seq     = nlh->nlmsg_seq;
    msgtype = nlh->nlmsg_type;

    debug("msg type: %d, from pid: %d, msg size: %d",
            msgtype, pid, nlh->nlmsg_len);
    if ( pid == 0 ) {
        printk(KERN_ERR "netlink_recv: message from kernel");
        return;
    }

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
    } else if ( msgtype > MSGT_FO_START && msgtype < MSGT_FO_END ) {
        debug("file opration message");
        data = skb_copy(skb, GFP_KERNEL);/* we will use this skb for ACK*/
        /* crate a new thread so server doesn't wait for ACK */
        task = kthread_run(&fo_recv, data, "fo_recv");
        if (IS_ERR(task)) {
            printk(KERN_ERR
                    "fo_recv: failed to create thread, error = %ld\n",
                    PTR_ERR(task));
            err = PTR_ERR(task); /* failure */
        }
    } else {
        debug("unknown message type: %d", msgtype);
    }

    /* if error or if this message says it wants a response */
    if ( err || (nlh->nlmsg_flags & NLM_F_ACK )) {
        prnttime();
        netlink_ack(skb, nlh, err); /* err should be 0 when no error */
        prnttime();
        debug("SENT ACK, err = %d", err);
    } else {
        /* if we don't send ack we have to free sk_buff */
        debug("not sending ACK");
        dev_kfree_skb(skb);
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

    NETLINK_CB(skb).portid = nddata->nlpid;
    NETLINK_CB(skb).dst_group = 0; /* not in mcast group */

    if (buff) { /* possible to send message without payload */
        memcpy(nlmsg_data(nlh) ,buff ,bufflen);
    }

    /* send messsage,nlmsg_unicast will take care of freeing skb */
    rvalue = nlmsg_unicast(nl_sk, skb, nddata->nlpid);

    /* TODO get confirmation of delivery */

    debug("rvalue = %d", rvalue);
    if(rvalue < 0) {
        printk(KERN_INFO "netlink_send: error while sending back to user\n");
        return -1; /* failure */
    }

    return 0; /* success */
free_skb:
    nlmsg_free(skb);
    return -1;
}

int netlink_send_skb(
    struct netdev_data *nddata,
    struct sk_buff *skb)
{
    int rvalue;

    if (!skb) {
        printk(KERN_ERR "netlink_send: skb is null\n");
        return -1;
    }

    debug("msg type: %d, to pid: %d, msg size: %d",
            nlmsg_hdr(skb)->nlmsg_type,
            nddata->nlpid,
            nlmsg_hdr(skb)->nlmsg_len);
    debug("skb->len = %d", skb->len);

    NETLINK_CB(skb).portid = nddata->nlpid;

    /* send messsage,nlmsg_unicast will take care of freeing skb */
    prnttime();
    rvalue = nlmsg_unicast(nl_sk, skb, nddata->nlpid);
    prnttime();

    /* TODO get confirmation of delivery */
    debug("rvalue = %d", rvalue);
    if(rvalue < 0) {
        printk(KERN_INFO "netlink_send: error while sending back to user\n");
        return -1; /* failure */
    }

    return 0; /* success */
}

struct sk_buff * netlink_pack_skb(
    struct nlmsghdr *nlh,
    void *buff,
    size_t bufflen)
{
    struct sk_buff *skb;

    skb = alloc_skb(NLMSG_SPACE(bufflen), GFP_KERNEL);
    if (!skb) {
        printk(KERN_ERR "fo_exectue: failed to expand skb\n");
        return NULL;
    }

    nlh = nlmsg_put(
                    skb,
                    nlh->nlmsg_pid,
                    nlh->nlmsg_seq,
                    nlh->nlmsg_type,
                    bufflen,
                    NLM_F_ACK);
    if (!nlh) {
        printk(KERN_ERR "netlink_pack_skb: insufficient tailroom\n");
        return NULL;
    }

    memcpy(nlmsg_data(nlh), buff, bufflen);

    return skb; /* success */
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

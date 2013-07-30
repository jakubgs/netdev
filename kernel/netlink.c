#include <linux/netlink.h>  /* for netlink sockets */
#include <linux/module.h>
#include <net/sock.h>

#include "netlink.h"
#include "netdevmgm.h"
#include "protocol.h"

struct sock *p_nl_sk = NULL;
/* TODO this will have to be part of a struct that will contain information
 * about the device this driver will pretend to be received from the server
 * process */
int pid = 0;

void netlink_recv(struct sk_buff *p_skb) {
    /* TODO this will have to be modified to receive responses from
     * server process, find the appropriate wait queue and release it
     * once the reply is assigned to appropriate variables in struct
     * describing one pending file operaion */
    struct nlmsghdr *p_nlh;
    short msgtype;

    p_nlh = (struct nlmsghdr*)p_skb->data;

    /* pid of sending process, also, port id */
    pid     = p_nlh->nlmsg_pid;
    msgtype = p_nlh->nlmsg_type;

    printk(KERN_DEBUG "netlink: msg type: %d, from pid: %d\n", msgtype, pid);
    if ( pid == 0 ) { /* message from kernel to kernel, disregard */
        printk(KERN_ERR "netlink: received message from kernel, pid: %d\n", pid);
        return;
    }

    /* if msgtype is a netdev control message */
    if ( msgtype >= MSGT_CONTROL_START && msgtype < MSGT_CONTROL_END ) {
        switch (msgtype) {  /* handle the request */
            case MSGT_CONTROL_ECHO:
                netlink_echo(pid, (char*)nlmsg_data(p_nlh));
                break;
            case MSGT_CONTROL_VERSION:
                break;
            case MSGT_CONTROL_REGISTER:
                ndmgm_create(pid, "netdev");
                break;
            case MSGT_CONTROL_UNREGISTER:
                ndmgm_find_destroy(pid);
                break;
            case MSGT_CONTROL_RECOVER:
                break;
            case MSGT_CONTROL_DRIVER_STOP:
                break;
            case MSGT_CONTROL_LOSTCONN:
                break;
            default:
                printk(KERN_ERR
                        "netlink: unknown control message type: %d\n",
                        msgtype);
                break;
        }
    /* if it's not a control message it has to be a file operation */
    } else if ( msgtype > MSGT_FO_START && msgtype < MSGT_FO_END ) {
        printk(KERN_DEBUG "netlink: file opration message type: %d\n",
                            msgtype);
        /* TODO reply to a file operation, find it in the queue and
         * release from it's wait */
    } else { /* otherwise we don't know what this message type is */
        printk(KERN_DEBUG "netlink: unknown message type: %d\n",
                            msgtype);
    }
}

int netlink_send(struct netdev_data *nddata, short msgtype, char *buff, size_t bufflen) {
    /* TODO should receive not only msgtype but also a buffer with
     * properly formatted arguments from netdev_fo_OP_send_req */
    struct nlmsghdr *p_nlh;
    struct sk_buff *p_skb_out;
    char *p_msg;
    int msg_size;
    int rvalue;

    printk(KERN_DEBUG "netlink_send: nddata = %p\n", nddata);

    /* don't send anything if we don't have the pid yet,
     * should be impossible since first process needs to
     * register a device before it can send events */
    if ( pid == 0 ) {
        printk(KERN_ERR "netlink: wrong pid: %d\n", pid);
        return -1;
    }

    if (msgtype == MSGT_CONTROL_ECHO) {
        p_msg = "ECHO";
        msg_size = strlen(p_msg);
    } else if (msgtype > MSGT_FO_START && msgtype < MSGT_FO_END) {
        p_msg = buff;
        msg_size = bufflen;
    } else {
        p_msg = "Not yet implemented!";
        msg_size = strlen(p_msg);
    }

    p_skb_out = nlmsg_new(msg_size, 0);

    if(!p_skb_out) {
        printk(KERN_ERR "netlink: failed to allocate new sk_buff!\n");
        goto skbfree;
    } 
    p_nlh = nlmsg_put(p_skb_out, 0, 0, msgtype, msg_size, 0);  
    p_nlh->nlmsg_pid = nddata->nlpid;

    NETLINK_CB(p_skb_out).dst_group = 0; /* not in mcast group */

    strncpy(nlmsg_data(p_nlh) ,p_msg ,msg_size);
    
    /* send messsage,nlmsg_unicast will take care of freeing p_skb_out */
    rvalue = nlmsg_unicast(p_nl_sk, p_skb_out, pid);

    if( rvalue > 0 ) {
        printk(KERN_INFO "Error while sending bak to user\n");
        return -1;
    }

    return 0; /* success */
free_skb:
    nlmsg_free(p_skb_out);
    return -1;
}

int netlink_send_fo(struct netdev_data *nddata, struct fo_req *req) {
    /* TODO should receive not only msgtype but also a buffer with
     * properly formatted arguments from netdev_fo_OP_send_req */
    struct nlmsghdr *p_nlh;
    struct sk_buff *p_skb_out;
    int rvalue;

    /* allocate space for message header and it's payload */
    p_skb_out = nlmsg_new(req->size, GFP_KERNEL);

    if(!p_skb_out) {
        printk(KERN_ERR "netlink: failed to allocate new sk_buff!\n");
        goto free_skb;
    } 

    /* increment the sequence number */
    if (!(req->seq = ndmgm_incseq(nddata))) {
        printk(KERN_ERR "netlink_send_fo: failed to increment sequence number\n");
        goto free_skb;
    }

    /* set pid, seq, message type, payload size and flags */
    p_nlh = nlmsg_put(
                    p_skb_out,
                    nddata->nlpid,
                    req->seq,
                    req->msgtype,
                    req->size,
                    NLM_F_REQUEST);

    NETLINK_CB(p_skb_out).dst_group = 0; /* not in mcast group */

    /* copy data to head of message payload */
    strncpy(nlmsg_data(p_nlh) ,req->args ,req->size);
    
    /* send messsage,nlmsg_unicast will take care of freeing p_skb_out */
    rvalue = nlmsg_unicast(p_nl_sk, p_skb_out, pid);

    if( rvalue > 0 ) {
        printk(KERN_INFO "Error while sending bak to user\n");
        return 0;
    }

    return 1; /* success */
free_skb:
    nlmsg_free(p_skb_out);
    return 0;
}

int netlink_echo(int pid, int seq, char *p_msg) {
    struct nlmsghdr *p_nlh;
    struct sk_buff *p_skb_out;
    int msg_size = strlen(p_msg);
    int rvalue;

    printk(KERN_INFO "netlink: received msg payload: %s\n", p_msg);

    /* allocate memory for sk_buff, no flags */
    p_skb_out = nlmsg_new(msg_size, GFP_KERNEL);

    if(!p_skb_out) {
        printk(KERN_ERR "netlink: failed to allocate new sk_buff!\n");
        rvalue = -1;
        goto skbfree;
    } 

    /* set pid, seq, message type, payload size and flags */
    p_nlh = nlmsg_put(
                    p_skb_out,
                    pid,
                    seq,
                    MSGT_CONTROL_ECHO,
                    msg_size,
                    NLM_F_ACK);  

    NETLINK_CB(p_skb_out).dst_group = 0; /* not in mcast group */

    strncpy(nlmsg_data(p_nlh) ,p_msg ,msg_size);

    /* send messsage, nlmsg_unicast will take care of freeing p_skb_out */
    rvalue = nlmsg_unicast(p_nl_sk,p_skb_out,pid);

    if(rvalue>0) {
        printk(KERN_ERR "netlink: error while replying to process\n");
        return -1;
    }

    return 0; /* success */
skbfree:
    nlmsg_free(p_skb_out);
    return -1;
}

int netlink_init(void) {
    /* NOTE: new structure for configuring netlink socket changed in
     * linux kernel 3.8
     * http://stackoverflow.com/questions/15937395/netlink-kernel-create-is-not-working-with-latest-linux-kernel
     */
    struct netlink_kernel_cfg nl_cfg;
    nl_cfg.groups = 0;           // used for multicast
    nl_cfg.flags = 0;            // TODO
    nl_cfg.input = netlink_recv; // function that will receive data

    printk("netdev: Netlink protocol: %d\n", NETLINK_PROTOCOL);

    p_nl_sk = netlink_kernel_create(&init_net,
                                    NETLINK_PROTOCOL,
                                    &nl_cfg);

    if(!p_nl_sk) {
        printk(KERN_ALERT "Error creating socket.\n");
        return -10;
    }

    printk("netdev: Netlink setup complete\n");
    return 0;
}

void netlink_exit(void) {
    printk(KERN_INFO "netdev: closing Netlink\n");
    /* TODO will have to close connections of all devices with the server
     * process or just close the socket if each device will have it's own
     * process/socket */

    /* can only be done once the process releases the socket, otherwise
     * kernel will refuse to unload the module */
    netlink_kernel_release(p_nl_sk);
}

MODULE_LICENSE("GPL");

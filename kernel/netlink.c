#include <linux/module.h>
#include <linux/netlink.h>  /* for netlink sockets */
#include <net/sock.h>

#include "netlink.h"
#include "protocol.h"

struct sock *p_nl_sk = NULL;
/* TODO this will have to be part of a struct that will contain information
 * about the device this driver will pretend to be received from the server
 * process */
int pid = 0;

int netlink_echo(int pid, char *p_msg) {
    struct nlmsghdr *p_nlh;
    struct sk_buff *p_skb_out;
    int msg_size = strlen(p_msg);
    int rvalue;

    /* allocate memory for sk_buff, no flags */
    p_skb_out = nlmsg_new(msg_size, 0);

    if(!p_skb_out) {
        printk(KERN_ERR "netlink: Failed to allocate new skb\n");
        return -1;
    } 
    p_nlh = nlmsg_put(p_skb_out, 0, 0, NLMSG_DONE, msg_size, 0);  

    NETLINK_CB(p_skb_out).dst_group = 0; /* not in mcast group */

    strncpy(nlmsg_data(p_nlh) ,p_msg ,msg_size);

    rvalue = nlmsg_unicast(p_nl_sk,p_skb_out,pid);

    if(rvalue>0) {
        printk(KERN_INFO "netlink: Error while sending bak to user\n");
        return -1;
    }

    return 1; /* success */
}

/* TODO this will have to be modified to receive responses from server process,
 * find the appropriate wait queue and release it once the reply is assigned
 * to appropriate variables in struct describing one pending file operaion */
void netlink_recv(struct sk_buff *skb) {
    struct nlmsghdr *p_nlh;
    short msgtype;

    printk(KERN_INFO "netlink: Entering: %s\n", __FUNCTION__);

    p_nlh = (struct nlmsghdr*)skb->data;
    printk(KERN_INFO "netlink: received msg payload: %s\n", (char*)nlmsg_data(p_nlh));

    pid = p_nlh->nlmsg_pid; /*pid of sending process */
    msgtype = p_nlh->nlmsg_type;

    printk(KERN_DEBUG "netlink: msg from pid = %d\n", pid);
    printk(KERN_DEBUG "netlink: msg type: %d\n", msgtype);
    if ( pid == 0 ) { /* message from kernel to kernel, disregard */
        return;
    }

    /* if msgtype is a netdev control message */
    if ( msgtype >= MSGTYPE_CONTROL_START && msgtype < MSGTYPE_CONTROL_END ) {
        switch (msgtype) {  /* handle the request */
            case MSGTYPE_CONTROL_ECHO:
                netlink_echo(pid, (char*)nlmsg_data(p_nlh));
                break;
            case MSGTYPE_CONTROL_REGISTER:
                break;
            case MSGTYPE_CONTROL_UNREGISTER:
                break;
            default:
                printk(KERN_WARNING "netlink: unknown control message type: %d\n", msgtype);
                break;
        }
    /* if it's not a control message it has to be a file operation */
    } else if ( msgtype > MSGTYPE_FO_START && msgtype < MSGTYPE_FO_END ) {
        printk(KERN_DEBUG "netlink: file opration message type: %d\n", msgtype);
        /* TODO reply to a file operation, queue it for the fo thread */
    } else { /* otherwise we don't know what this message type is */
        printk(KERN_DEBUG "netlink: unknown message type: %d\n", msgtype);
    }
}

/* TODO should receive not only msgtype but also a buffer with properly 
 * formatted arguments to the file operation from the netdev_fo_OP_send_req */
int netlink_send(short msgtype) {
    struct nlmsghdr *p_nlh;
    struct sk_buff *p_skb_out;
    char *p_msg;
    int msg_size;
    int rvalue;

    if ( pid == -1 ) { /* don't send anything if we don't have the pid yet */
        return -1;
    }

    if (msgtype == MSGTYPE_CONTROL_ECHO) {
        p_msg = "ECHO";
    } else {
        p_msg = "DEFAULT";
    }
    msg_size = strlen(p_msg);

    p_skb_out = nlmsg_new(msg_size, 0);

    if(!p_skb_out) {
        printk(KERN_ERR "Failed to allocate new skb\n");
        return -1;
    } 
    p_nlh = nlmsg_put(p_skb_out, 0, 0, NLMSG_DONE, msg_size, 0);  

    NETLINK_CB(p_skb_out).dst_group = 0; /* not in mcast group */

    strncpy(nlmsg_data(p_nlh) ,p_msg ,msg_size);

    rvalue = nlmsg_unicast(p_nl_sk, p_skb_out, pid);

    if(rvalue>0)
        printk(KERN_INFO "Error while sending bak to user\n");

    return 0;
}

int netlink_init(void) {
    // new structure for configuring netlink socket changed in linux kernel 3.8
    // http://stackoverflow.com/questions/15937395/netlink-kernel-create-is-not-working-with-latest-linux-kernel
    struct netlink_kernel_cfg nl_cfg;
    nl_cfg.groups = 0;           // used for multicast
    nl_cfg.flags = 0;            // TODO
    nl_cfg.input = netlink_recv; // pointer to function that will receive data

    printk("netdev: Netlink protocol: %d\n", NETLINK_PROTOCOL);

    p_nl_sk = netlink_kernel_create(&init_net, NETLINK_PROTOCOL, &nl_cfg);

    if(!p_nl_sk) {
        printk(KERN_ALERT "Error creating socket.\n");
        return -10;
    }

    printk("netdev: Netlink setup complete\n");
    return 0;
}

void netlink_exit(void) {
    printk(KERN_INFO "netdev: closing Netlink\n");
    // TODO will have to close connections of all devices with the server process

    /* can only be done once the process releases the socket, otherwise
     * kernel will refuse to unload the module */
    netlink_kernel_release(p_nl_sk);
}

MODULE_LICENSE("GPL");

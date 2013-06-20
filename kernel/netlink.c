#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>

#include "netlink.h"

#define NETLINK_USER 31

struct sock *nl_sk = NULL;

void hello_nl_recv_msg(struct sk_buff *skb) {
    struct nlmsghdr *nlh;
    int pid;
    struct sk_buff *skb_out;
    int msg_size;
    char *msg="Hello from kernel";
    int res;

    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);

    msg_size=strlen(msg);

    nlh=(struct nlmsghdr*)skb->data;
    printk(KERN_INFO "Netlink received msg payload: %s\n",(char*)nlmsg_data(nlh));
    pid = nlh->nlmsg_pid; /*pid of sending process */

    skb_out = nlmsg_new(msg_size,0);

    if(!skb_out)
    {

        printk(KERN_ERR "Failed to allocate new skb\n");
        return;

    } 
    nlh = nlmsg_put(skb_out,0,0,NLMSG_DONE,msg_size,0);  

    NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */

    strncpy(nlmsg_data(nlh),msg,msg_size);

    res=nlmsg_unicast(nl_sk,skb_out,pid);

    if(res>0)
        printk(KERN_INFO "Error while sending bak to user\n");

}

int init_netlink(void) {
    // new structure for configuring netlink socket changed in linux kernel 3.8
    // http://stackoverflow.com/questions/15937395/netlink-kernel-create-is-not-working-with-latest-linux-kernel
    struct netlink_kernel_cfg nl_cfg;
    nl_cfg.groups = 0,// used for multicast
    nl_cfg.flags = 0, // TODO
    nl_cfg.input = hello_nl_recv_msg, // pointer to function that will send data

    printk("Entering: %s\n",__FUNCTION__);

    nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &nl_cfg);

    if(!nl_sk)
    {

        printk(KERN_ALERT "Error creating socket.\n");
        return -10;

    }

    return 0;
}

void exit_netlink(void) {
    printk(KERN_INFO "exiting hello module\n");
    netlink_kernel_release(nl_sk);
}

MODULE_LICENSE("GPL");
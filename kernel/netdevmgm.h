#ifndef _NETDEVMGM_H
#define _NETDEVMGM_H

#include <linux/cdev.h>
#include <linux/module.h>   /* for THIS_MODULE */
#include <linux/hashtable.h>

#include "protocol.h"

extern dev_t netdev_devno; /* __u32, unsigned 32bit type, 12 bit for majro, 20 minor */
extern struct class *netdev_class;

/* for now we use only one, but in future we will use one for every device */
/* TODO this structure will need a semafor/spinlock */
struct netdev_data {
    /* TODO might need to use uint32_t for the sake of network communication */
    unsigned int nlpid; /* pid of the process that created this device */
    char *devname; /* name of the device provided by the process */
    struct cdev *cdev; /* internat struct representing char devices */
    struct device *device;
    bool ready; /* set to false if netlink connection is lost */
    struct hlist_node hnode; /* to use with hastable */
    /* TODO
     * this struct will containt more data especially about host of the device
     * and device name, major, minor, and process pid of the server that will
     * be communicating through netlink
     */
};

int netdev_create(int nlpid, char *name);
int netdev_destroy(int nlpid, struct netdev_data *nddata);
void netdev_htable_init(void);
struct netdev_data* netdev_find(int nlpid);
void netdev_end(void);

#endif



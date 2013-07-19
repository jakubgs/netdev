#ifndef _NETDEV_H
#define _NETDEV_H

#include <linux/cdev.h>
#include <linux/module.h>   /* for THIS_MODULE */

/* for now we use only one, but in future we will use one for every device */
/* TODO this structure will need a semafor/spinlock */
struct netdev_data {
    /* TODO might need to use uint32_t for the sake of network communication */
    unsigned int nlpid; /* pid of the process that created this device */
    char *name; /* name of the device provided by the process */
    struct cdev cdev;
    struct device *device;
    /* TODO
     * this struct will containt more data especially about host of the device
     * and device name, major, minor, and process pid of the server that will
     * be communicating through netlink
     */
};

int netdev_create(int nlpid, char *name);

#endif

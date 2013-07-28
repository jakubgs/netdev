#ifndef _NETDEVMGM_H
#define _NETDEVMGM_H

#include <linux/cdev.h>
#include <linux/module.h>   /* for THIS_MODULE */
#include <linux/hashtable.h>
#include <linux/rwsem.h>
#include <linux/kfifo.h>
#include <linux/slab.h>

#include "protocol.h"

extern dev_t netdev_devno; /* __u32, unsigned 32bit type, 12 bit for majro, 20 minor */
extern struct class *netdev_class;
extern unsigned int netdev_count;
extern struct netdev_data **netdev_devices; /* array with all devices */

/* for now we use only one, but in future we will use one for every device */
/* TODO this structure will need a semafor/spinlock */
struct netdev_data {
    /* TODO might need to use uint32_t for the sake of network communication */
    unsigned int nlpid; /* pid of the process that created this device */
    bool active; /* set to false if netlink connection is lost */
    char *devname; /* name of the device provided by the process */
    struct cdev *cdev; /* internat struct representing char devices */
    struct device *device;
    struct hlist_node hnode; /* to use with hastable */
    struct rw_semaphore sem; /* necessary since many threads can use this */
    struct kfifo fo_queue; /* queue for file operations */
    struct kmem_cache *queue_pool; /* pool of memory for fo_req structs */
    /* TODO
     * this struct will containt more data especially about host of the
     * device and device name, major, minor, and process pid of the server
     * that will be communicating through netlink */
};

struct netdev_data * ndmgm_alloc_data(int nlpid, char *name);
int ndmgm_free_data(struct netdev_data *nddata);
int ndmgm_create(int nlpid, char *name);
int ndmgm_find_destroy(int nlpid);
int ndmgm_destroy(struct netdev_data *nddata);
int ndmgm_end(void);
void ndmgm_prepare(void);
struct netdev_data* ndmgm_find(int nlpid);

#endif



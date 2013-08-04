#ifndef _NETDEVMGM_H
#define _NETDEVMGM_H

#include <linux/cdev.h>
#include <linux/module.h>   /* for THIS_MODULE */
#include <linux/hashtable.h>
#include <linux/rwsem.h>
#include <linux/kfifo.h>
#include <linux/slab.h>

#include "protocol.h"
#include "fo_comm.h"

struct fo_req;
extern dev_t netdev_devno; /* __u32, unsigned 32bit type, 12 bit for majro, 20 minor */
extern struct class *netdev_class;
extern unsigned int netdev_count;
extern int *netdev_minors_used; /* array with pids to all the devices */

/* for now we use only one, but in future we will use one for every device */
/* TODO this structure will need a semafor/spinlock */
struct netdev_data {
    /* TODO might need to use uint32_t for the sake of network communication */
    unsigned int nlpid; /* pid of the process that created this device */
    bool active; /* set to false if netlink connection is lost */
    bool dummy; /* defines if this is a server device or a dummy one */
    char *devname; /* name of the device provided by the process */
    atomic_t users; /* counter of threads using this objecto */
    atomic_t curseq; /* last used sequence number for this device, 0 is not used */
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
int ndmgm_create(int nlpid, char *name);
struct netdev_data * ndmgm_find(int nlpid);
void ndmgm_free_data(struct netdev_data *nddata);
int ndmgm_free_queue(struct netdev_data *nddata);
struct fo_req * ndmgm_foreq_find(struct netdev_data *nddata, int seq);
int ndmgm_foreq_add(struct netdev_data *nddata, struct fo_req *req);
int ndmgm_incseq(struct netdev_data *nddata);
int ndmgm_find_destroy(int nlpid);
int ndmgm_destroy(struct netdev_data *nddata);
int ndmgm_end(void);
void ndmgm_prepare(void);
void ndmgm_get(struct netdev_data *nddata);
void ndmgm_put(struct netdev_data *nddata);
int ndmgm_refs(struct netdev_data *nddata);

#endif



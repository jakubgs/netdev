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
/* __u32, unsigned 32bit type, 12 bit for majro, 20 minor */
extern dev_t netdev_devno;
extern struct class *netdev_class;
extern unsigned int netdev_count;
extern int *netdev_minors_used; /* array with pids to all the devices */

struct netdev_data {
    unsigned int nlpid; /* pid of the process that created this device */
    bool active; /* set to false if netlink connection is lost */
    bool dummy; /* defines if this is a server device or a dummy one */
    char *devname; /* name of the device provided by the process */
    atomic_t users; /* counter of threads using this objecto */
    atomic_t curseq; /* last used sequence number for this device */
    spinlock_t nllock; /* netlink lock ensuring proper order for ACK msg */
    struct cdev *cdev; /* internat struct representing char devices */
    struct device *device;
    struct hlist_node hnode; /* to use with hastable */
    struct rw_semaphore sem; /* necessary since threads can use this */
    struct kmem_cache *queue_pool; /* pool of memory for fo_req structs */
    /* hash table for active open files */
    DECLARE_HASHTABLE(foacc_htable, NETDEV_HTABLE_ACC_SIZE);
};

struct netdev_data * ndmgm_alloc_data(int nlpid, char *name);
int ndmgm_create_dummy(int nlpid, char *name);
int ndmgm_create_server(int nlpid, char *name);
struct netdev_data * ndmgm_find(int nlpid);
void ndmgm_free_data(struct netdev_data *nddata);
int ndmgm_incseq(struct netdev_data *nddata);
struct fo_access * ndmgm_find_acc(struct netdev_data *nddata, int access_id);
int ndmgm_find_destroy(int nlpid);
int ndmgm_destroy(struct netdev_data *nddata);
int ndmgm_destroy_allacc( struct netdev_data *nddata);
int ndmgm_destroy_dummy(struct netdev_data *nddata);
int ndmgm_destroy_server(struct netdev_data *nddata);
int ndmgm_end(void);
void ndmgm_prepare(void);
void ndmgm_get(struct netdev_data *nddata);
void ndmgm_put(struct netdev_data *nddata);
int ndmgm_refs(struct netdev_data *nddata);

#endif

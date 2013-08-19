#ifndef _NETDEV_H
#define _NETDEV_H

/* this will be provided by the name variable in netdev_data struct */
#define NETDEV_NAME             "netdev"

/* let the kernel decide the major number */
#define NETDEV_MAJOR            0

/* defines the number which is the power of two which gives the max number
 * of devices this driver will manage and store in the hashtable */
#define NETDEV_HTABLE_DEV_SIZE  5
#define NETDEV_HTABLE_ACC_SIZE  5
#define NETDEV_MAX_DEVICES      32 /* 2 to the power of 5(HTABLE_DEV_SIZE)*/
#define NETDEV_MAX_OPEN_FILES   32 /* 2 to the power of 5(HTABLE_ACC_SIZE) */
#define NETDEV_FO_QUEUE_SIZE    64 /* has to be a power of 2 */
#define NETDEV_REQ_POOL_NAME    "netdev_req_pool" /* for debugging */

#endif /* _NETDEV_H */

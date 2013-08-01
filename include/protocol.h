#ifndef _PROTOCOL_H
#define _PROTOCOL_H

/* this will be provided by the name variable in netdev_data struct */
#define NETDEV_NAME             "netdev"

/* let the kernel decide the major number */
#define NETDEV_MAJOR            0

/* defines the number which is the power of two which gives the max number
 * of devices this driver will manage and store in the hashtable */
#define NETDEV_HASHTABLE_SIZE   5
#define NETDEV_MAX_DEVICES      32 /* 2 to the power of 5(HASHTABLE_SIE) */
#define NETDEV_FO_QUEUE_SIZE    64 /* has to be a power of 2 */
#define NETDEV_REQ_POOL_NAME    "netdev_req_pool" /* for debugging */
/* defines the default netlink server port */
#define NETDEV_SERVER_PORT      9999

#define NETDEV_MAXLINE          1024

/* 2nd argument to listen() */
#define NETDEV_LISTENQ          1024

/* defines the protocol used, we want our own protocol */
#define NETLINK_PROTOCOL        31

/* define type of message for netlink driver control,
 * will be used in nlmsghdr->nlmsg_type */
#define MSGT_CONTROL_ECHO         1 /* test to check if diver works */
#define MSGT_CONTROL_SUCCESS      2 /* test to check if diver works */
#define MSGT_CONTROL_FAILURE      3 /* test to check if diver works */
#define MSGT_CONTROL_VERSION      4 /* check if driver is compatible */
#define MSGT_CONTROL_REG_DUMMY    5 /* register a new device */
#define MSGT_CONTROL_REG_SERVER   6 /* register a new device */
#define MSGT_CONTROL_UNREGISTER   7 /* unregister the device */
#define MSGT_CONTROL_RECOVER      8 /* recover a device after lost conn. */
#define MSGT_CONTROL_DRIVER_STOP  9 /* close all devices and stop driver */
#define MSGT_CONTROL_LOSTCONN     10 /* lost connection with other end */

/* limit of control messsages */
#define MSGT_CONTROL_START        0
#define MSGT_CONTROL_END          100

/* Definitions used to distinguis between file operation structures
 * that contain all the functiona arguments, best used with short or __u16
 * Will be either used in nlmsghdr->nlmsg_type or some inner variable */
#define MSGT_FO_LLSEEK            101
#define MSGT_FO_READ              102
#define MSGT_FO_WRITE             103
#define MSGT_FO_AIO_READ          104
#define MSGT_FO_AIO_WRITE         105
#define MSGT_FO_READDIR           106
#define MSGT_FO_POLL              107
#define MSGT_FO_UNLOCKED_IOCTL    108
#define MSGT_FO_COMPAT_IOCTL      109
#define MSGT_FO_MMAP              110
#define MSGT_FO_OPEN              111
#define MSGT_FO_FLUSH             112
#define MSGT_FO_RELEASE           113
#define MSGT_FO_FSYNC             114
#define MSGT_FO_AIO_FSYNC         115
#define MSGT_FO_FASYNC            116
#define MSGT_FO_LOCK              117
#define MSGT_FO_SENDPAGE          118
#define MSGT_FO_GET_UNMAPPED_AREA 119
#define MSGT_FO_CHECK_FLAGS       120
#define MSGT_FO_FLOCK             121
#define MSGT_FO_SPLICE_WRITE      122
#define MSGT_FO_SPLICE_READ       123
#define MSGT_FO_SETLEASE          124
#define MSGT_FO_FALLOCATE         125
#define MSGT_FO_SHOW_FDINF        126

#define MSGT_FO_START             100
#define MSGT_FO_END               127

/* for sake of convenience and shorter lines */
#define SA struct sockaddr

#endif /* _PROTOCOL_H */

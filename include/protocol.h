#ifndef _PROTOCOL_H
#define _PROTOCOL_H

/* this number HAS TO BE incremented every time anything in the 
 * the core protocol is changed */
#define NETDEV_PROTOCOL_VERSION 2
#define NETDEV_PROTOCOL_SUCCESS 0 /* 0 will always mean correct */
#define NETDEV_PROTOCOL_FAILURE -1 /* -1 will always mean failure */

/* size limit of messages sent through sendmsg */
#define NETDEV_MESSAGE_LIMIT    213000

/* defines the default netlink server port */
#define NETDEV_SERVER_PORT      9999

/* 2nd argument to listen(), max size of listen queue */
#define NETDEV_LISTENQ          1024

/* defines the netlink bus used, we want our own protocol so we use the 
 * last one of the 32 available buses */
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
#define MSGT_CONTROL_END          11

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

#endif /* _PROTOCOL_H */

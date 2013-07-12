#ifndef _MSGTYPE_H
#define _MSGTYPE_H

/* defines the protocol used, we want our own protocol */
#define NETLINK_PROTOCOL 31

/* define type of message for netlink driver management,
 * will be used in nlmsghdr->nlmsg_type */
#define MSGTYPE_NETDEV_ECHO         0
#define MSGTYPE_NETDEV_REGISTER     1
#define MSGTYPE_NETDEV_UNREGISTER   2
#define MSGTYPE_NETDEV_DRIVER_STOP  3

/* Definitions used to distinguis between file operation structures
 * that contain all the functiona arguments, best used with short or __u16
 * Will be either used in nlmsghdr->nlmsg_type or some inner variable */
#define MSGTYPE_FO_LLSEEK            101
#define MSGTYPE_FO_READ              102
#define MSGTYPE_FO_WRITE             103
#define MSGTYPE_FO_AIO_READ          104
#define MSGTYPE_FO_AIO_WRITE         105
#define MSGTYPE_FO_READDIR           106
#define MSGTYPE_FO_POLL              107
#define MSGTYPE_FO_UNLOCKED_IOCTL    108
#define MSGTYPE_FO_COMPAT_IOCTL      109
#define MSGTYPE_FO_MMAP              110
#define MSGTYPE_FO_OPEN              111
#define MSGTYPE_FO_FLUSH             112
#define MSGTYPE_FO_RELEASE           113
#define MSGTYPE_FO_FSYNC             114
#define MSGTYPE_FO_AIO_FSYNC         115
#define MSGTYPE_FO_FASYNC            116
#define MSGTYPE_FO_LOCK              117
#define MSGTYPE_FO_SENDPAGE          118
#define MSGTYPE_FO_GET_UNMAPPED_AREA 119
#define MSGTYPE_FO_CHECK_FLAGS       120
#define MSGTYPE_FO_FLOCK             121
#define MSGTYPE_FO_SPLICE_WRITE      122
#define MSGTYPE_FO_SPLICE_READ       123
#define MSGTYPE_FO_SETLEASE          124
#define MSGTYPE_FO_FALLOCATE         125
#define MSGTYPE_FO_SHOW_FDINF        126

#endif /* _MSGTYPE_H */

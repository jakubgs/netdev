#ifndef _FO_RECV_H
#define _FO_RECV_H

#include "fo_comm.h"

/* functions for executing file operations sent from client */
int netdev_fo_llseek_recv_req (struct fo_req *req);
int netdev_fo_read_recv_req (struct fo_req *req);
int netdev_fo_write_recv_req (struct fo_req *req);
int netdev_fo_aio_read_recv_req (struct fo_req *req);
int netdev_fo_aio_write_recv_req (struct fo_req *req);
int netdev_fo_poll_recv_req (struct fo_req *req);
int netdev_fo_unlocked_ioctl_recv_req (struct fo_req *req);
int netdev_fo_compat_ioctl_recv_req (struct fo_req *req);
int netdev_fo_mmap_recv_req (struct fo_req *req);
int netdev_fo_open_recv_req (struct fo_req *req);
int netdev_fo_flush_recv_req (struct fo_req *req);
int netdev_fo_release_recv_req (struct fo_req *req);
int netdev_fo_fsync_recv_req (struct fo_req *req);
int netdev_fo_aio_fsync_recv_req (struct fo_req *req);
int netdev_fo_fasync_recv_req (struct fo_req *req);
int netdev_fo_lock_recv_req (struct fo_req *req);
int netdev_fo_sendpage_recv_req (struct fo_req *req);
int netdev_fo_get_unmapped_area_recv_req (struct fo_req *req);
int netdev_fo_check_flags_recv_req (struct fo_req *req);
int netdev_fo_flock_recv_req (struct fo_req *req);
int netdev_fo_splice_write_recv_req (struct fo_req *req);
int netdev_fo_splice_read_recv_req (struct fo_req *req);
int netdev_fo_setlease_recv_req (struct fo_req *req);
int netdev_fo_fallocate_recv_req (struct fo_req *req);
int netdev_fo_show_fdinfo_recv_req (struct fo_req *req);

extern int (*netdev_recv_fops[])(struct fo_req*);

#endif /* _FO_RECV_H */

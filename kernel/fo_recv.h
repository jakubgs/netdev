#ifndef _FO_RECV_H
#define _FO_RECV_H

#include "fo_comm.h"
#include "fo_access.h"

/* functions for executing file operations sent from client */
int ndfo_recv_llseek(struct fo_access *acc, struct fo_req *req);
int ndfo_recv_read(struct fo_access *acc, struct fo_req *req);
int ndfo_recv_write(struct fo_access *acc, struct fo_req *req);
int ndfo_recv_aio_read(struct fo_access *acc, struct fo_req *req);
int ndfo_recv_aio_write(struct fo_access *acc, struct fo_req *req);
int ndfo_recv_poll(struct fo_access *acc, struct fo_req *req);
int ndfo_recv_unlocked_ioctl(struct fo_access *acc, struct fo_req *req);
int ndfo_recv_compat_ioctl(struct fo_access *acc, struct fo_req *req);
int ndfo_recv_mmap(struct fo_access *acc, struct fo_req *req);
int ndfo_recv_open(struct fo_access *acc, struct fo_req *req);
int ndfo_recv_flush(struct fo_access *acc, struct fo_req *req);
int ndfo_recv_release(struct fo_access *acc, struct fo_req *req);
int ndfo_recv_fsync(struct fo_access *acc, struct fo_req *req);
int ndfo_recv_aio_fsync(struct fo_access *acc, struct fo_req *req);
int ndfo_recv_fasync(struct fo_access *acc, struct fo_req *req);
int ndfo_recv_lock(struct fo_access *acc, struct fo_req *req);
int ndfo_recv_sendpage(struct fo_access *acc, struct fo_req *req);
int ndfo_recv_get_unmapped_area(struct fo_access *acc, struct fo_req *req);
int ndfo_recv_check_flags(struct fo_access *acc, struct fo_req *req);
int ndfo_recv_flock(struct fo_access *acc, struct fo_req *req);
int ndfo_recv_splice_write(struct fo_access *acc, struct fo_req *req);
int ndfo_recv_splice_read(struct fo_access *acc, struct fo_req *req);
int ndfo_recv_setlease(struct fo_access *acc, struct fo_req *req);
int ndfo_recv_fallocate(struct fo_access *acc, struct fo_req *req);
int ndfo_recv_show_fdinfo(struct fo_access *acc, struct fo_req *req);

extern int (*netdev_recv_fops[])(struct fo_access*, struct fo_req*);

#endif /* _FO_RECV_H */

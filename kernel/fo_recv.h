#ifndef _FO_RECV_H
#define _FO_RECV_H

#include "fo_comm.h"

/* functions for executing file operations sent from client */
int ndfo_recv_llseek(struct netdev_data *nddata, struct fo_req *req);
int ndfo_recv_read(struct netdev_data *nddata, struct fo_req *req);
int ndfo_recv_write(struct netdev_data *nddata, struct fo_req *req);
int ndfo_recv_aio_read(struct netdev_data *nddata, struct fo_req *req);
int ndfo_recv_aio_write(struct netdev_data *nddata, struct fo_req *req);
int ndfo_recv_poll(struct netdev_data *nddata, struct fo_req *req);
int ndfo_recv_unlocked_ioctl(struct netdev_data *nddata, struct fo_req *req);
int ndfo_recv_compat_ioctl(struct netdev_data *nddata, struct fo_req *req);
int ndfo_recv_mmap(struct netdev_data *nddata, struct fo_req *req);
int ndfo_recv_open(struct netdev_data *nddata, struct fo_req *req);
int ndfo_recv_flush(struct netdev_data *nddata, struct fo_req *req);
int ndfo_recv_release(struct netdev_data *nddata, struct fo_req *req);
int ndfo_recv_fsync(struct netdev_data *nddata, struct fo_req *req);
int ndfo_recv_aio_fsync(struct netdev_data *nddata, struct fo_req *req);
int ndfo_recv_fasync(struct netdev_data *nddata, struct fo_req *req);
int ndfo_recv_lock(struct netdev_data *nddata, struct fo_req *req);
int ndfo_recv_sendpage(struct netdev_data *nddata, struct fo_req *req);
int ndfo_recv_get_unmapped_area(struct netdev_data *nddata, struct fo_req *req);
int ndfo_recv_check_flags(struct netdev_data *nddata, struct fo_req *req);
int ndfo_recv_flock(struct netdev_data *nddata, struct fo_req *req);
int ndfo_recv_splice_write(struct netdev_data *nddata, struct fo_req *req);
int ndfo_recv_splice_read(struct netdev_data *nddata, struct fo_req *req);
int ndfo_recv_setlease(struct netdev_data *nddata, struct fo_req *req);
int ndfo_recv_fallocate(struct netdev_data *nddata, struct fo_req *req);
int ndfo_recv_show_fdinfo(struct netdev_data *nddata, struct fo_req *req);

extern int (*netdev_recv_fops[])(struct netdev_data*, struct fo_req*);

#endif /* _FO_RECV_H */

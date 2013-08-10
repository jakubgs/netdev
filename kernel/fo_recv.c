#include "fo_recv.h"

int netdev_fo_llseek_recv_req (struct fo_req *req) {
    return -1;
}

int netdev_fo_read_recv_req (struct fo_req *req) {
    return -1;
}

int netdev_fo_write_recv_req (struct fo_req *req) {
    return -1;
}

int netdev_fo_aio_read_recv_req (struct fo_req *req) {
    return -1;
}

int netdev_fo_aio_write_recv_req (struct fo_req *req) {
    return -1;
}

int netdev_fo_poll_recv_req (struct fo_req *req) {
    return -1;
}

int netdev_fo_unlocked_ioctl_recv_req (struct fo_req *req) {
    return -1;
}

int netdev_fo_compat_ioctl_recv_req (struct fo_req *req) {
    return -1;
}

int netdev_fo_mmap_recv_req (struct fo_req *req) {
    return -1;
}

int netdev_fo_open_recv_req (struct fo_req *req) {
    return -1;
}

int netdev_fo_flush_recv_req (struct fo_req *req) {
    return -1;
}

int netdev_fo_release_recv_req (struct fo_req *req) {
    return -1;
}

int netdev_fo_fsync_recv_req (struct fo_req *req) {
    return -1;
}

int netdev_fo_aio_fsync_recv_req (struct fo_req *req) {
    return -1;
}

int netdev_fo_fasync_recv_req (struct fo_req *req) {
    return -1;
}

int netdev_fo_lock_recv_req (struct fo_req *req) {
    return -1;
}

int netdev_fo_sendpage_recv_req (struct fo_req *req) {
    return -1;
}

int netdev_fo_get_unmapped_area_recv_req (struct fo_req *req) {
    return -1;
}

int netdev_fo_check_flags_recv_req (struct fo_req *req) {
    return -1;
}

int netdev_fo_flock_recv_req (struct fo_req *req) {
    return -1;
}

int netdev_fo_splice_write_recv_req (struct fo_req *req) {
    return -1;
}

int netdev_fo_splice_read_recv_req (struct fo_req *req) {
    return -1;
}

int netdev_fo_setlease_recv_req (struct fo_req *req) {
    return -1;
}

int netdev_fo_fallocate_recv_req (struct fo_req *req) {
    return -1;
}

int netdev_fo_show_fdinfo_recv_req (struct fo_req *req) {
    return -1;
}

int (*netdev_recv_fops[])(struct fo_req*) = {
    netdev_fo_llseek_recv_req,
    netdev_fo_read_recv_req,
    netdev_fo_write_recv_req,
    NULL, /* will be hard to implement because of kiocb */
    NULL, /* will be hard to implement because of kiocb */
    NULL, /* useless for devices */
    netdev_fo_poll_recv_req,/* NULL assumes it's not blocing */
    netdev_fo_unlocked_ioctl_recv_req,
    netdev_fo_compat_ioctl_recv_req,
    netdev_fo_mmap_recv_req,
    netdev_fo_open_recv_req,
    netdev_fo_flush_recv_req, /* rarely used for devices */
    netdev_fo_release_recv_req,
    netdev_fo_fsync_recv_req,
    NULL, /* will be hard to implement because of kiocb */
    netdev_fo_fasync_recv_req,
    netdev_fo_lock_recv_req, /* almost never implemented */
    netdev_fo_sendpage_recv_req, /* usually not used */
    netdev_fo_get_unmapped_area_recv_req, /* mostly NULL */
    netdev_fo_check_flags_recv_req, /* fcntl(F_SETFL...) */
    netdev_fo_flock_recv_req,
    netdev_fo_splice_write_recv_req, /* needs DMA */
    netdev_fo_splice_read_recv_req, /* needs DMA */
    netdev_fo_setlease_recv_req,
    netdev_fo_fallocate_recv_req,
    netdev_fo_show_fdinfo_recv_req
};

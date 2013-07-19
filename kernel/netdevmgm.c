#include "netdev.h"
#include "fo.h"

struct file_operations netdev_fops = {
    .owner             = THIS_MODULE,
    .llseek            = netdev_fo_llseek,
    .read              = netdev_fo_read,
    .write             = netdev_fo_write,
    .aio_read          = netdev_fo_aio_read,
    .aio_write         = netdev_fo_aio_write,
    .readdir           = netdev_fo_readdir,
    .poll              = netdev_fo_poll,
    .unlocked_ioctl    = netdev_fo_unlocked_ioctl,
    .compat_ioctl      = netdev_fo_compat_ioctl,
    .mmap              = netdev_fo_mmap,
    .open              = netdev_fo_open,
    .flush             = netdev_fo_flush,
    .release           = netdev_fo_release,
    .fsync             = netdev_fo_fsync,
    .aio_fsync         = netdev_fo_aio_fsync,
    .fasync            = netdev_fo_fasync,
    .lock              = netdev_fo_lock,
    .sendpage          = netdev_fo_sendpage,
    .get_unmapped_area = netdev_fo_get_unmapped_area,
    .check_flags       = netdev_fo_check_flags,
    .flock             = netdev_fo_flock,
    .splice_write      = netdev_fo_splice_write,
    .splice_read       = netdev_fo_splice_read,
    .setlease          = netdev_fo_setlease,
    .fallocate         = netdev_fo_fallocate,
    .show_fdinfo       = netdev_fo_show_fdinfo
};


int netdev_create(int nlpid, char *name) {
    return 1;
}



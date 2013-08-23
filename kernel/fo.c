#include <linux/module.h>

#include "fo.h"
#include "fo_send.h"

/* TODO this struct will have to be generated dynamically based on the
 * operations supported by the device on the other end */
struct file_operations netdev_fops = {
    .owner             = THIS_MODULE,
    .llseek            = ndfo_send_llseek,
    .read              = ndfo_send_read,
    .write             = ndfo_send_write,
    .aio_read          = NULL, /* will be hard to implement because of kiocb */
    .aio_write         = NULL, /* will be hard to implement because of kiocb */
    .readdir           = NULL, /* useless for devices */
    .poll              = ndfo_send_poll,/* NULL assumes it's not blocing */
    .unlocked_ioctl    = ndfo_send_unlocked_ioctl,
    .compat_ioctl      = ndfo_send_compat_ioctl,
    .mmap              = NULL, /* for mapping device memory to user space */
    .open              = ndfo_send_open,
    .flush             = NULL, /* rarely used for devices */
    .release           = ndfo_send_release,
    .fsync             = ndfo_send_fsync,
    .aio_fsync         = NULL, /* will be hard to implement because of kiocb */
    .fasync            = ndfo_send_fasync,
    .lock              = NULL, /* almost never implemented */
    .sendpage          = NULL, /* usually not used, for sockets */
    .get_unmapped_area = NULL, /* mostly NULL */
    .check_flags       = ndfo_send_check_flags, /* fcntl(F_SETFL...) */
    .flock             = ndfo_send_flock,
    .splice_write      = NULL, /* needs DMA */
    .splice_read       = NULL, /* needs DMA */
    .setlease          = ndfo_send_setlease,
    .fallocate         = ndfo_send_fallocate,
    .show_fdinfo       = ndfo_send_show_fdinfo
};

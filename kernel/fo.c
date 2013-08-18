#include "fo.h"
#include <linux/sched.h>    /* access to current->comm and current->pid */
#include <linux/module.h>

#include "netdevmgm.h"
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
    .lock              = ndfo_send_lock, /* almost never implemented */
    .sendpage          = ndfo_send_sendpage, /* usually not used */
    .get_unmapped_area = ndfo_send_get_unmapped_area, /* mostly NULL */
    .check_flags       = ndfo_send_check_flags, /* fcntl(F_SETFL...) */
    .flock             = ndfo_send_flock,
    .splice_write      = ndfo_send_splice_write, /* needs DMA */
    .splice_read       = ndfo_send_splice_read, /* needs DMA */
    .setlease          = ndfo_send_setlease,
    .fallocate         = ndfo_send_fallocate,
    .show_fdinfo       = ndfo_send_show_fdinfo
};

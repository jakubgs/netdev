#include "fo_send.h"
#include "fo_comm.h"
#include "fo_struct.h"
#include "protocol.h"
#include "dbg.h"

/* functions for sending and receiving file operations */
loff_t netdev_fo_llseek_send_req (struct file *filp, loff_t offset, int whence)
{
    return -EIO;
}
ssize_t netdev_fo_read_send_req (struct file *filp, char __user *data, size_t size, loff_t *offset)
{
    struct s_fo_read args = {
        .data = data,
        .size = size,
        .offset = offset
    };

    if ( fo_send(MSGT_FO_READ,
                filp->private_data,
                &args, sizeof(args),
                data, 0) )
    {
        return args.rvalue;
    }
    return -EIO;
}
ssize_t netdev_fo_write_send_req (struct file *filp, const char __user *data, size_t size, loff_t *offset)
{
    struct s_fo_write args = {
        .data = data,
        .size = size,
        .offset = offset
    };

    if ( fo_send(MSGT_FO_WRITE,
                filp->private_data,
                &args, sizeof(args),
                (void*)data, size)) {
        return args.rvalue;
    }
    return -EIO;
}
size_t netdev_fo_aio_read_send_req (struct kiocb *a, const struct iovec *b, unsigned long c, loff_t offset)
{
    return -EIO;
}
ssize_t netdev_fo_aio_write_send_req (struct kiocb *a, const struct iovec *b, unsigned long c, loff_t d)
{
    return -EIO;
}
unsigned int netdev_fo_poll_send_req (struct file *filp, struct poll_table_struct *wait)
{
    return -EIO;
}
long netdev_fo_unlocked_ioctl_send_req (struct file *filp, unsigned int b, unsigned long c)
{
    return -EIO;
}
long netdev_fo_compat_ioctl_send_req (struct file *filp, unsigned int b, unsigned long c)
{
    return -EIO;
}
int netdev_fo_mmap_send_req (struct file *filp, struct vm_area_struct *b)
{
    return -EIO;
}
int netdev_fo_open_send_req (struct inode *inode, struct file *filp)
{
    struct s_fo_open args =
    {
        .inode = inode
    };

    args.rvalue = -EIO; /* TEST */

    if ( fo_send(MSGT_FO_OPEN,
                filp->private_data,
                &args, sizeof(args),
                NULL, 0) )
    {
        debug("rvalue = %d", args.rvalue);
        return args.rvalue;
    }
    return -EIO;
}
int netdev_fo_flush_send_req (struct file *filp, fl_owner_t id)
{
    fo_send(MSGT_FO_FLUSH,
            filp->private_data,
            NULL, 0,
            NULL, 0);
    return -EIO;
}
int netdev_fo_release_send_req (struct inode *a, struct file *filp)
{
    fo_send(MSGT_FO_RELEASE,
            filp->private_data,
            NULL, 0,
            NULL, 0);
    return -EIO;
}
int netdev_fo_fsync_send_req (struct file *filp, loff_t b, loff_t offset, int d)
{
    return -EIO;
}
int netdev_fo_aio_fsync_send_req (struct kiocb *a, int b)
{
    return -EIO;
}
int netdev_fo_fasync_send_req (int a, struct file *filp, int c)
{
    return -EIO;
}
int netdev_fo_lock_send_req (struct file *filp, int b, struct file_lock *c)
{
    return -EIO;
}
ssize_t netdev_fo_sendpage_send_req (struct file *filp, struct page *b, int c, size_t d, loff_t *offset, int f)
{
    return -EIO;
}
unsigned long netdev_fo_get_unmapped_area_send_req (struct file *filp, unsigned long b, unsigned long c,unsigned long d, unsigned long e)
{
    return -EIO;
}
int netdev_fo_check_flags_send_req (int a)
{
    return -EIO;
}
int netdev_fo_flock_send_req (struct file *filp, int b, struct file_lock *c)
{
    return -EIO;
}
ssize_t netdev_fo_splice_write_send_req (struct pipe_inode_info *a, struct file *filp, loff_t *offset, size_t d, unsigned int e)
{
    return -EIO;
}
ssize_t netdev_fo_splice_read_send_req (struct file *filp, loff_t *offset, struct pipe_inode_info *c, size_t d, unsigned int e)
{
    return -EIO;
}
int netdev_fo_setlease_send_req (struct file *filp, long b, struct file_lock **c)
{
    return -EIO;
}
long netdev_fo_fallocate_send_req (struct file *filp, int b, loff_t offset, loff_t len)
{
    return -EIO;
}
int netdev_fo_show_fdinfo_send_req (struct seq_file *a, struct file *filp)
{
    return -EIO;
}

#include "fo_send.h"
#include "fo_comm.h"
#include "fo_struct.h"
#include "protocol.h"
#include "dbg.h"

/* functions for sending and receiving file operations */
loff_t ndfo_send_llseek(struct file *filp, loff_t offset, int whence)
{
    loff_t rvalue = 0;
    struct s_fo_llseek args = {
        .offset = offset,
        .whence = whence,
        .rvalue = -EIO
    };

    rvalue = fo_send(MSGT_FO_LLSEEK,
                    filp->private_data,
                    &args, sizeof(args),
                    NULL, 0);

    if (rvalue < 0) {
        debug("rvalue = %lld", rvalue);
        return rvalue;
    }
    return args.rvalue;
}
ssize_t ndfo_send_read(struct file *filp, char __user *data, size_t size, loff_t *offset)
{
    ssize_t rvalue = 0;
    struct s_fo_read args = {
        .size = size,
        .offset = offset,
        .rvalue = -EIO
    };
    if (size >= NETDEV_MESSAGE_LIMIT) {
        printk(KERN_ERR "ndfo_send_read: buffor too big for message\n");
        return -EINVAL;
    }
    args.data = kmalloc(size, GFP_KERNEL);
    if (!args.data) {
        printk(KERN_ERR "ndfo_send_read: failed to allocate args.data\n");
        return -ENOMEM;
    }

    rvalue = fo_send(MSGT_FO_READ,
                    filp->private_data,
                    &args, sizeof(args),
                    args.data, 0);

    if (rvalue < 0) {
        debug("rvalue = %zu", rvalue);
        return rvalue;
    }
    rvalue = copy_to_user(data, args.data, args.rvalue);
    if (rvalue > 0) {
        debug("rvalue = %zu", rvalue);
        printk(KERN_ERR "ndfo_send_read: failed to copy to user\n");
        return -1;
    }
    return args.rvalue;
}
ssize_t ndfo_send_write(struct file *filp, const char __user *data, size_t size, loff_t *offset)
{
    size_t rvalue = 0;
    struct s_fo_write args = {
        .size = size,
        .offset = offset,
        .rvalue = -EIO
    };
    if (size >= NETDEV_MESSAGE_LIMIT) {
        printk(KERN_ERR "ndfo_send_write: buffor too big for message\n");
        return -EINVAL;
    }
    args.data = kmalloc(size, GFP_KERNEL);
    if (!args.data) {
        printk(KERN_ERR "ndfo_send_write: failed to allocate args.data\n");
        return -ENOMEM;
    }

    rvalue = copy_from_user(args.data, data, size);
    if (rvalue > 0) {
        debug("rvalue = %zu", rvalue);
        printk(KERN_ERR "ndfo_send_write: failed to copy from user\n");
        return -EIO;
    }

    rvalue = fo_send(MSGT_FO_WRITE,
                    filp->private_data,
                    &args, sizeof(args),
                    args.data, size);

    if (rvalue < 0) {
        debug("rvalue = %zu", rvalue);
        return rvalue;
    }
    return args.rvalue;
}
size_t ndfo_send_aio_read(struct kiocb *a, const struct iovec *b, unsigned long c, loff_t offset)
{
    return -EIO;
}
ssize_t ndfo_send_aio_write(struct kiocb *a, const struct iovec *b, unsigned long c, loff_t d)
{
    return -EIO;
}
unsigned int ndfo_send_poll(struct file *filp, struct poll_table_struct *wait)
{
    return -EIO;
}
long ndfo_send_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long rvalue = 0;
    struct s_fo_unlocked_ioctl args = {
        .cmd = cmd,
        .arg = arg,
        .rvalue = -EIO
    };

    rvalue = fo_send(MSGT_FO_UNLOCKED_IOCTL,
                    filp->private_data,
                    &args, sizeof(args),
                    NULL, 0);

    if (rvalue < 0) {
        debug("rvalue = %ld", rvalue);
        return rvalue;
    }
    return args.rvalue;
}
long ndfo_send_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long rvalue = 0;
    struct s_fo_compat_ioctl args = {
        .cmd = cmd,
        .arg = arg,
        .rvalue = -EIO
    };

    rvalue = fo_send(MSGT_FO_COMPAT_IOCTL,
                    filp->private_data,
                    &args, sizeof(args),
                    NULL, 0);

    if (rvalue < 0) {
        debug("rvalue = %ld", rvalue);
        return rvalue;
    }
    return args.rvalue;
}
int ndfo_send_mmap(struct file *filp, struct vm_area_struct *b)
{
    return -EIO;
}
int ndfo_send_open(struct inode *inode, struct file *filp)
{
    int rvalue = 0;
    struct s_fo_open args = {
        .inode = inode,
        .rvalue = -EIO
    };

    rvalue = fo_send(MSGT_FO_OPEN,
                    filp->private_data,
                    &args, sizeof(args),
                    NULL, 0);

    if (rvalue < 0) {
        debug("rvalue = %d", rvalue);
        return rvalue;
    }
    return args.rvalue;
}
int ndfo_send_flush(struct file *filp, fl_owner_t id)
{
    int rvalue = 0;
    struct s_fo_flush args = {
        .rvalue = -EIO
    };

    rvalue = fo_send(MSGT_FO_FLUSH,
                    filp->private_data,
                    &args, sizeof(args),
                    NULL, 0);

    if (rvalue < 0) {
        debug("rvalue = %d", rvalue);
        return rvalue;
    }
    return args.rvalue;
}
int ndfo_send_release(struct inode *a, struct file *filp)
{
    int rvalue = 0;
    struct s_fo_release args = {
        .rvalue = -EIO
    };

    rvalue = fo_send(MSGT_FO_RELEASE,
                    filp->private_data,
                    &args, sizeof(args),
                    NULL, 0);

    if (rvalue < 0) {
        debug("rvalue = %d", rvalue);
        return rvalue;
    }
    return args.rvalue;
}
int ndfo_send_fsync(struct file *filp, loff_t b, loff_t offset, int d)
{
    return -EIO;
}
int ndfo_send_aio_fsync(struct kiocb *a, int b)
{
    return -EIO;
}
int ndfo_send_fasync(int a, struct file *filp, int c)
{
    return -EIO;
}
int ndfo_send_lock(struct file *filp, int b, struct file_lock *c)
{
    return -EIO;
}
ssize_t ndfo_send_sendpage(struct file *filp, struct page *b, int c, size_t d, loff_t *offset, int f)
{
    return -EIO;
}
unsigned long ndfo_send_get_unmapped_area(struct file *filp, unsigned long b, unsigned long c,unsigned long d, unsigned long e)
{
    return -EIO;
}
int ndfo_send_check_flags(int a)
{
    return -EIO;
}
int ndfo_send_flock(struct file *filp, int b, struct file_lock *c)
{
    return -EIO;
}
ssize_t ndfo_send_splice_write(struct pipe_inode_info *a, struct file *filp, loff_t *offset, size_t d, unsigned int e)
{
    return -EIO;
}
ssize_t ndfo_send_splice_read(struct file *filp, loff_t *offset, struct pipe_inode_info *c, size_t d, unsigned int e)
{
    return -EIO;
}
int ndfo_send_setlease(struct file *filp, long b, struct file_lock **c)
{
    return -EIO;
}
long ndfo_send_fallocate(struct file *filp, int b, loff_t offset, loff_t len)
{
    return -EIO;
}
int ndfo_send_show_fdinfo(struct seq_file *a, struct file *filp)
{
    return -EIO;
}

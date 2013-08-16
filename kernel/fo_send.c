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
        .data = data,
        .size = size,
        .offset = offset,
        .rvalue = -EIO
    };

    rvalue = fo_send(MSGT_FO_READ,
                    filp->private_data,
                    &args, sizeof(args),
                    data, 0);

    if (rvalue < 9) {
        debug("rvalue = %zu", rvalue);
        return rvalue;
    }
    return args.rvalue;
}
ssize_t ndfo_send_write(struct file *filp, const char __user *data, size_t size, loff_t *offset)
{
    size_t rvalue = 0;
    struct s_fo_write args = {
        .data = data,
        .size = size,
        .offset = offset,
        .rvalue = -EIO
    };

    rvalue = fo_send(MSGT_FO_WRITE,
                    filp->private_data,
                    &args, sizeof(args),
                    (void*)data, size);

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
long ndfo_send_unlocked_ioctl(struct file *filp, unsigned int b, unsigned long c)
{
    return -EIO;
}
long ndfo_send_compat_ioctl(struct file *filp, unsigned int b, unsigned long c)
{
    return -EIO;
}
int ndfo_send_mmap(struct file *filp, struct vm_area_struct *b)
{
    return -EIO;
}
int ndfo_send_open(struct inode *inode, struct file *filp)
{
    int rvalue = 0;
    struct s_fo_open args =
    {
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
    debug("args.rvalue = %d", args.rvalue);
    return args.rvalue;
}
int ndfo_send_flush(struct file *filp, fl_owner_t id)
{
    int rvalue = 0;

    rvalue = fo_send(MSGT_FO_FLUSH,
                    filp->private_data,
                    NULL, 0,
                    NULL, 0);

    if (rvalue < 0) {
        debug("rvalue = %d", rvalue);
        return rvalue;
    }
    return -EIO;
}
int ndfo_send_release(struct inode *a, struct file *filp)
{
    fo_send(MSGT_FO_RELEASE,
            filp->private_data,
            NULL, 0,
            NULL, 0);
    return -EIO;
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

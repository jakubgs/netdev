#include <linux/fs.h>
#include <linux/fcntl.h>

#include "fo_recv.h"
#include "fo_struct.h"
#include "dbg.h"

int ndfo_recv_llseek(struct netdev_data *nddata, struct fo_req *req) {
    struct s_fo_llseek *args = req->args;

    if (!nddata->filp->f_op->llseek) {
        printk(KERN_ERR "ndfo_recv_llseek: operation is NULL\n");
        return -1;
    }
    args->rvalue = nddata->filp->f_op->llseek(nddata->filp,
                                            args->offset,
                                            args->whence);

    return 0;
}

int ndfo_recv_read(struct netdev_data *nddata, struct fo_req *req) {
    struct s_fo_read *args = req->args;

    req->data = kzalloc(args->size , GFP_KERNEL);
    req->data_size = args->size;

    args->rvalue = nddata->filp->f_op->read(nddata->filp,
                                            req->data,
                                            args->size,
                                            args->offset);

    debug("args->rvalue = %zu", args->rvalue);
    return 0;
}

int ndfo_recv_write(struct netdev_data *nddata, struct fo_req *req) {
    struct s_fo_write *args = req->args;

    req->data_size = args->size;

    args->rvalue = nddata->filp->f_op->write(nddata->filp,
                                            req->data,
                                            args->size,
                                            args->offset);

    return 0;
}

int ndfo_recv_aio_read(struct netdev_data *nddata, struct fo_req *req) {
    return -1;
}

int ndfo_recv_aio_write(struct netdev_data *nddata, struct fo_req *req) {
    return -1;
}

int ndfo_recv_poll(struct netdev_data *nddata, struct fo_req *req) {
    return -1;
}

int ndfo_recv_unlocked_ioctl(struct netdev_data *nddata, struct fo_req *req) {
    return -1;
}

int ndfo_recv_compat_ioctl(struct netdev_data *nddata, struct fo_req *req) {
    return -1;
}

int ndfo_recv_mmap(struct netdev_data *nddata, struct fo_req *req) {
    return -1;
}

int ndfo_recv_open(struct netdev_data *nddata, struct fo_req *req) {
    struct s_fo_open *args = req->args;
    int err = 0;
    int flags = O_RDWR | O_LARGEFILE;
    int mode = 0;

    nddata->filp = filp_open(nddata->devname, flags, mode);

    debug("filp_open executed");
    if (IS_ERR(nddata->filp)) {
        err = PTR_ERR(nddata->filp);
        printk(KERN_ERR "ndfo_recv_open_req: err = %d\n", err);
        return -1; /* failure */
    }
    debug("nddata->filp = %p", nddata->filp);

    if (!nddata->filp->f_op) {
        printk(KERN_ERR "ndfo_recv_open_req: no file operations\n");
        return -1; /* failure */
    }

    args->rvalue = 0;
    debug("success");
    return 0; /* success */
}

int ndfo_recv_flush(struct netdev_data *nddata, struct fo_req *req) {
    struct s_fo_flush *args = req->args;

    if (!nddata->filp->f_op->flush) {
        printk(KERN_ERR "ndfo_recv_flush: operation is NULL\n");
        return -1;
    }
    args->rvalue = nddata->filp->f_op->flush(nddata->filp, NULL);

    return 0;
}

int ndfo_recv_release(struct netdev_data *nddata, struct fo_req *req) {
    struct s_fo_release *args = req->args;

    args->rvalue = filp_close(nddata->filp, NULL);

    return 0;
}

int ndfo_recv_fsync(struct netdev_data *nddata, struct fo_req *req) {
    return -1;
}

int ndfo_recv_aio_fsync(struct netdev_data *nddata, struct fo_req *req) {
    return -1;
}

int ndfo_recv_fasync(struct netdev_data *nddata, struct fo_req *req) {
    return -1;
}

int ndfo_recv_lock(struct netdev_data *nddata, struct fo_req *req) {
    return -1;
}

int ndfo_recv_sendpage(struct netdev_data *nddata, struct fo_req *req) {
    return -1;
}

int ndfo_recv_get_unmapped_area(struct netdev_data *nddata, struct fo_req *req) {
    return -1;
}

int ndfo_recv_check_flags(struct netdev_data *nddata, struct fo_req *req) {
    return -1;
}

int ndfo_recv_flock(struct netdev_data *nddata, struct fo_req *req) {
    return -1;
}

int ndfo_recv_splice_write(struct netdev_data *nddata, struct fo_req *req) {
    return -1;
}

int ndfo_recv_splice_read(struct netdev_data *nddata, struct fo_req *req) {
    return -1;
}

int ndfo_recv_setlease(struct netdev_data *nddata, struct fo_req *req) {
    return -1;
}

int ndfo_recv_fallocate(struct netdev_data *nddata, struct fo_req *req) {
    return -1;
}

int ndfo_recv_show_fdinfo(struct netdev_data *nddata, struct fo_req *req) {
    return -1;
}

int (*netdev_recv_fops[])(struct netdev_data *, struct fo_req*) = {
    ndfo_recv_llseek,
    ndfo_recv_read,
    ndfo_recv_write,
    NULL, /* will be hard to implement because of kiocb */
    NULL, /* will be hard to implement because of kiocb */
    NULL, /* useless for devices */
    ndfo_recv_poll,/* NULL assumes it's not blocing */
    ndfo_recv_unlocked_ioctl,
    ndfo_recv_compat_ioctl,
    ndfo_recv_mmap,
    ndfo_recv_open,
    ndfo_recv_flush, /* rarely used for devices */
    ndfo_recv_release,
    ndfo_recv_fsync,
    NULL, /* will be hard to implement because of kiocb */
    ndfo_recv_fasync,
    ndfo_recv_lock, /* almost never implemented */
    ndfo_recv_sendpage, /* usually not used */
    ndfo_recv_get_unmapped_area, /* mostly NULL */
    ndfo_recv_check_flags, /* fcntl(F_SETFL...) */
    ndfo_recv_flock,
    ndfo_recv_splice_write, /* needs DMA */
    ndfo_recv_splice_read, /* needs DMA */
    ndfo_recv_setlease,
    ndfo_recv_fallocate,
    ndfo_recv_show_fdinfo
};

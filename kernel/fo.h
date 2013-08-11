#ifndef _FO_H
#define _FO_H

#include <linux/fs.h> /* iovec, kiocb, file, pool_table_struct */

/* file operation functions for file_operations structure */
loff_t ndfo_llseek (struct file *flip, loff_t offset, int whence);
ssize_t ndfo_read (struct file *flip, char __user *data, size_t c, loff_t *offset);
ssize_t ndfo_write (struct file *flip, const char __user *data, size_t c, loff_t *offset);
ssize_t ndfo_aio_read (struct kiocb *a, const struct iovec *b, unsigned long c, loff_t offset);
ssize_t ndfo_aio_write (struct kiocb *a, const struct iovec *b, unsigned long c, loff_t d);
unsigned int ndfo_poll (struct file *filp, struct poll_table_struct *wait);
long ndfo_unlocked_ioctl (struct file *filp, unsigned int b, unsigned long c);
long ndfo_compat_ioctl (struct file *filp, unsigned int b, unsigned long c);
int ndfo_mmap (struct file *filp, struct vm_area_struct *b);
int ndfo_open (struct inode *inode, struct file *filp);
int ndfo_flush (struct file *filp, fl_owner_t id);
int ndfo_release (struct inode *a, struct file *filp);
int ndfo_fsync (struct file *filp, loff_t b, loff_t offset, int d);
int ndfo_aio_fsync (struct kiocb *a, int b);
int ndfo_fasync (int a, struct file *filp, int c);
int ndfo_lock (struct file *filp, int b, struct file_lock *c);
ssize_t ndfo_sendpage (struct file *filp, struct page *b, int c, size_t size, loff_t *offset, int f);
unsigned long ndfo_get_unmapped_area (struct file *filp, unsigned long b, unsigned long c,unsigned long d, unsigned long e);
int ndfo_check_flags (int a);
int ndfo_flock (struct file *filp, int b, struct file_lock *c);
ssize_t ndfo_splice_write (struct pipe_inode_info *a, struct file *filp, loff_t *offset, size_t size, unsigned int e);
ssize_t ndfo_splice_read (struct file *filp, loff_t *offset, struct pipe_inode_info *c, size_t size, unsigned int e);
int ndfo_setlease (struct file *filp, long b, struct file_lock **c);
long ndfo_fallocate (struct file *filp, int b, loff_t offset, loff_t len);
int ndfo_show_fdinfo (struct seq_file *a, struct file *filp);

extern struct file_operations netdev_fops;

#endif /* _FO_H */

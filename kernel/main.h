#ifndef _NETDEV_MAIN_H
#define _NETDEV_MAIN_H

loff_t  netdev_fo_llseek(struct file *filp, loff_t offset, int whence);
ssize_t netdev_fo_read(struct file *filp, char __user *data, size_t c, loff_t *offset);
ssize_t netdev_fo_write(struct file *filp, const char __user *data, size_t c, loff_t *offset);
ssize_t netdev_fo_aio_read ( struct kiocb *a, const struct iovec *b, unsigned long c, loff_t offset);
ssize_t netdev_fo_aio_write ( struct kiocb *a, const struct iovec *b, unsigned long c, loff_t offset);
int     netdev_fo_readdir ( struct file *filp, void *b, filldir_t c);
unsigned int netdev_fo_poll ( struct file *filp, struct poll_table_struct *wait);
long    netdev_fo_unlocked_ioctl ( struct file *filp, unsigned int b, unsigned long c);
long    netdev_fo_compat_ioctl ( struct file *filp, unsigned int b, unsigned long c);
int     netdev_fo_mmap ( struct file *filp, struct vm_area_struct *b);
int     netdev_fo_open ( struct inode *inode, struct file *filp);
int     netdev_fo_flush ( struct file *filp, fl_owner_t id);
int     netdev_fo_release ( struct inode *a, struct file *b);
int     netdev_fo_fsync ( struct file *filp, loff_t b, loff_t c, int d);
int     netdev_fo_aio_fsync ( struct kiocb *a, int b);
int     netdev_fo_fasync ( int a, struct file *b, int c);
int     netdev_fo_lock ( struct file *filp, int b, struct file_lock *c);
ssize_t netdev_fo_sendpage ( struct file *filp, struct page *b, int c, size_t d, loff_t *offset, int f);
unsigned long netdev_fo_get_unmapped_area( struct file *filp, unsigned long b, unsigned long c, unsigned long d, unsigned long e);
int     netdev_fo_check_flags( int a);
int     netdev_fo_flock ( struct file *filp, int b, struct file_lock *c);
ssize_t netdev_fo_splice_write( struct pipe_inode_info *a, struct file *filp, loff_t *offset, size_t d, unsigned int e);
ssize_t netdev_fo_splice_read( struct file *filp, loff_t *offset, struct pipe_inode_info *c, size_t d, unsigned int e);
int     netdev_fo_setlease( struct file *filp, long b, struct file_lock **c);
long    netdev_fo_fallocate( struct file *filp, int b, loff_t offset, loff_t len);
int netdev_fo_show_fdinfo( struct seq_file *a, struct file *filp);

#endif /* _NETDEV_MAIN_H */

#ifndef _FO_STRUCT_H
#define _FO_STRUCT_H

/* structures used to pass file operation arguments to sending function */
/* TODO add return values of functions as "rvalue" or something like that */
struct s_fo_llseek {
        struct file *flip;
            loff_t offset;
                int whence;
};
struct s_fo_read {
        struct file *flip;
            char __user *data;
                size_t c;
                    loff_t *offset;
};
struct s_fo_write {
        struct file *flip;
            const char __user *data;
                size_t c;
                    loff_t *offset;
};
struct s_fo_aio_read {
        struct kiocb *a;
            const struct iovec *b;
                unsigned long c;
                    loff_t offset;
};
struct s_fo_aio_write {
        struct kiocb *a;
            const struct iovec *b;
                unsigned long c;
                    loff_t offset;
};
struct s_fo_readdir {
        struct file *filp;
            void *b;
                filldir_t c;
};
struct s_dev_fo_poll {
        struct file *filp;
            struct poll_table_struct *wait;
};
struct s_fo_unlocked_ioctl {
        struct file *filp;
            unsigned int b;
                unsigned long c;
};
struct s_fo_compat_ioctl {
        struct file *filp;
            unsigned int b;
                unsigned long c;
};
struct s_fo_mmap {
        struct file *filp;
            struct vm_area_struct *b;
};
struct s_fo_open {
        struct inode *inode;
            struct file *filp;
};
struct s_fo_flush {
        struct file *filp;
            fl_owner_t id;
};
struct s_fo_release {
        struct inode *a;
            struct file *filp;
};
struct s_fo_fsync {
        struct file *filp;
            loff_t b;
                loff_t c;
                    int d;
};
struct s_fo_aio_fsync {
        struct kiocb *a;
            int b;
};
struct s_fo_fasync {
        int a;
            struct file *filp;
                int c;
};
struct s_fo_lock {
        struct file *filp;
            int b;
                struct file_lock *c;
};
struct s_fo_sendpage {
        struct file *filp;
            struct page *b;
                int c;
                    size_t d;
                        loff_t *offset;
                            int f;
};
struct s_fo_get_unmapped_area{
        struct file *filp;
            unsigned long b;
                unsigned long c;
                    unsigned long d;
                        unsigned long e;
};
struct s_fo_check_flags{
        int a;
};
struct s_fo_flock {
        struct file *filp;
            int b;
                struct file_lock *c;
};
struct s_fo_splice_write{
        struct pipe_inode_info *a;
            struct file *filp;
                loff_t *offset;
                    size_t d;
                        unsigned int e;
};
struct s_fo_splice_read{
        struct file *filp;
            loff_t *b;
                struct pipe_inode_info *c;
                    size_t d;
                        unsigned int e;
};
struct s_fo_setlease{
        long b;
            struct file *filp;
                struct file_lock **c;
};
struct s_fo_fallocate{
        struct file *filp;
            int b;
                loff_t offset;
                    loff_t len;
};
struct s_fo_show_fdinfo{
        struct seq_file *a;
            struct file *filp;
};

#endif /* _FO_STRUCT_H */

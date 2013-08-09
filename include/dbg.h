#ifndef _DBG_H
#define _DBG_H


#ifdef NDEBUG
#define debug(M, ...)
#else
#define debug(M, ...) printk(KERN_DEBUG "NETDEV_DEBUG: %20s:%4d:\t\t" M "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif

#endif /* _DBG_H */

#ifndef _NETPROTOCL_H
#define _NETPROTOCL_H

struct netdev_header {
    int msgtype;
    size_t size;
    void *payload;
};

#endif

#ifndef _NETPROTOCL_H
#define _NETPROTOCL_H

struct netdev_header {
    int msgtype;
    size_t size;
    void *payload;
};

void* serialise_netdev_header(struct netdev_header *ndhead);
struct netdev_header* deserialise_netdev_header(void *buffer, size_t size);

#endif

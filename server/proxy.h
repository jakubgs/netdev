#ifndef _PROXY_H
#define _PROXY_H

#include <linux/netlink.h>  /* sockaddr_nl */
#include <netinet/in.h>     /* sockaddr_in */
#include <sys/un.h>         /* sockaddr_un */

#define MAX_PAYLOAD 1024 /* maximum payload size*/

typedef enum {
    true,
    false
} bool;

struct proxy_dev {
    int pid; /* pid od the proxy process */
    int nl_fd; /* netlink file descryptor */
    int rm_fd; /* remote file descryptor */
    int us_fd[2]; /* unix socket pair, first one is for reading */
    bool client; /* defines if this is a client or a server */
    struct sockaddr_nl *nl_src_addr;
    struct sockaddr_nl *nl_dst_addr;
    struct sockaddr_in *rm_addr;
    struct sockaddr_un *us_addr;
    int rm_portaddr;
    char *rm_ipaddr;
    char *remote_dev_name;
    char *dummy_dev_name;
};

int proxy_netlink_setup(struct proxy_dev *pdev);
int proxy_register_device(struct proxy_dev *pdev, struct msghdr *p_msg, struct iovec *p_iov);
int proxy_unregister_device(void);
void proxy_setup_signals();
int proxy_setup_unixsoc(struct proxy_dev *pdev);
struct proxy_dev * proxy_alloc_dev(int connfd);
void proxy_client(struct proxy_dev *pdev);
void proxy_server(int connfd);
void proxy_sig_hup(int signo);
int proxy_loop();
int proxy_control(struct proxy_dev *pdev);
int proxy_handle_remote(struct proxy_dev *pdev);
int proxy_handle_netlink(struct proxy_dev *pdev);
int netlink_unregister_dev(struct proxy_dev *pdev);

#endif

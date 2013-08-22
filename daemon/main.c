#include <string.h>        /* memset, memcopy, memmove, memcmp */
#include <stdio.h>         /* printf                           */
#include <stdlib.h>        /* malloc                           */
#include <unistd.h>        /* close, getpid                    */
#include <sys/socket.h>    /* socklen_t */
#include <sys/types.h>     /* pid_t                            */
#include <sys/wait.h>      /* waitpid */
#include <arpa/inet.h>      /* inet_notp */
#include <errno.h>
#include <netinet/ip.h>
#include <signal.h>         /* signal */

#include "signal.h"
#include "proxy.h"
#include "protocol.h"
#include "debug.h"

void parent_sig_chld(int signo) {
    pid_t pid;
    int stat;
    printf("sig_chld: received signal\n");

    while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0 ) {
        /* TODO should't use printf in signal handling */
        printf("ALERT: child %d terminated\n", pid);
    }

    return;
}

int netdev_listener(
    int port)
{
    int listenfd, connfd;
    int rvalue;
    pid_t childpid;
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;
    char str[INET_ADDRSTRLEN];
    void *ptr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    if ( listenfd < 0 ) {
        perror("netdev_listener: failed to create socket");
        return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port   = htons(port);

    rvalue = bind(listenfd,
                (struct sockaddr*)&servaddr,
                sizeof(servaddr));

    if ( rvalue < 0 ) {
        perror("netdev_listener: failed to bind to address");
        return -1;
    }

    rvalue = listen(listenfd, NETDEV_LISTENQ);

    if ( rvalue < 0 ) {
        perror("netdev_listener: failed to listen");
        return -1;
    }

    printf("netdev_listener: starting listener at port: %d\n", port);
    while (1) {
        clilen = sizeof(cliaddr);

        if ((connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen)) < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                perror("netdev_listener: accept error");
            }
        }

        ptr = (char *)inet_ntop(cliaddr.sin_family ,
                                &cliaddr.sin_addr,
                                str,
                                sizeof(str));

        if ( ptr != NULL ) {
            printf("netdev_listener: new connection from %s:%d\n",
                    str,
                    ntohs(cliaddr.sin_port));
        }

        /* fork for every new client and serve it */
        if ( (childpid = fork()) ==0 ) {
            /* decrease the counter for listening socket,
             * this process won't use it */
            close(listenfd);

            proxy_server(connfd);

            exit(0);
        }

        /* decreas the counter for new connection */
        close(connfd);
    }

    return 0;
}

struct proxy_dev ** read_config(char *filename, int *count) {
    struct proxy_dev **pdevs = NULL;
    FILE *fp;
    char c;
    char line[100];
    int i = 0;
    int rvalue;
    *count = 0;

    if (filename == NULL) {
        return NULL;
    }

    fp = fopen(filename, "r");

    if (!fp) {
        perror("failed to open file");
        return NULL;
    }

    while (EOF != (fscanf(fp, "%c%*[^\n]", &c), fscanf(fp, "%*c"))) {
        if (c != '#') { /* ignore comments */
            ++*count;
        }
    }
    rewind(fp); /* go back to the beginning of the file */

    pdevs = malloc(*count * sizeof(struct proxy_dev*));
    memset(pdevs, 0, *count * sizeof(struct proxy_dev*));

    if (!pdevs) {
        perror("read_config: failed to allocate pdevs");
        return NULL; /* failure */
    }

    for (i = 0; i < *count; ) {
        fgets(line, sizeof(line), fp);
        if (line[0] == '#') { /* if first character is # it's a comment */
            continue;
        }

        if (!pdevs[i]) {
            pdevs[i] = malloc(sizeof(struct proxy_dev));
            memset(pdevs[i], 0, sizeof(struct proxy_dev));
            pdevs[i]->remote_dev_name = malloc(NETDEV_MAX_STRING);
            pdevs[i]->dummy_dev_name =  malloc(NETDEV_MAX_STRING);
            pdevs[i]->rm_ipaddr =       malloc(NETDEV_MAX_STRING);
        }

        pdevs[i]->client = true; /* this is a client instance */
        rvalue  = sscanf(line, "%20[^;];%20[^;];%20[^;];%d\n",
                        pdevs[i]->remote_dev_name,
                        pdevs[i]->dummy_dev_name,
                        pdevs[i]->rm_ipaddr,
                        &pdevs[i]->rm_portaddr);
        if (rvalue == EOF) {
            break;
        }
        if (rvalue < 4) {
            printf("failed to parse configuration in line %d\n", i+1);
            break;
        }
        i++;
    }
    *count = i;

    return pdevs; /* success */
}

int main(int argc, char *argv[]) {
    int pid, dev_count, i, port = NETDEV_DAEMON_PORT;
    char *conffile = NULL;
    char c;
    struct proxy_dev **pdevs = NULL;


    while ((c = getopt(argc, argv, ":hf:p:")) != -1) {
        switch(c) {
        case 'p':
            port = atoi(optarg);
            break;
        case 'f':
            conffile = optarg;
            break;
        case 'h':
            printf("usage: %s [-f CONFIGFILE] [-p SERVERPORT] [-h]\n", argv[0]);
            return 0;
        case ':':       /* -f or -o without operand */
            fprintf(stderr,
                "Option -%c requires an operand\n", optopt);
            return 1;
        case '?':
            fprintf(stderr,
                "Unrecognized option: '-%c'\n", optopt);
            return 1;
        }
    }

    /* to run waitpid for all forked proxies */
    signal(SIGCHLD, parent_sig_chld);

    if (!(pdevs = read_config(conffile, &dev_count))) {
        printf("failed to client read configuration\n");
    } else {
        for ( i = 0 ; i < dev_count ; i++ ) {
            if ( ( pid = fork() ) == 0 ) {
                proxy_client(pdevs[i]);
                exit(0);
            } else if ( pid < 0 ) {
                perror("main: failed to fork");
            }
        }
    }

    if ( netdev_listener(port) ) {
        printf("main: could not start listener\n");
    }

    return 0;
}

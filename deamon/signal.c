#include <sys/types.h>     /* pid_t                            */
#include <sys/wait.h>      /* waitpid                          */
#include <stdio.h>         /* TODO should use write when handling signal */
#include <signal.h>
#include <unistd.h>

#include "signal.h"

typedef void    Sigfunc(int);   /* for signal handlers */

Sigfunc * signal(int signo, Sigfunc *func) {
    struct sigaction act, oact;

    act.sa_handler = func;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    if (signo == SIGALRM) {
        act.sa_flags |= SA_RESTART;
    }

    if (sigaction(signo, &act, &oact) < 0)
        return SIG_ERR;

    return oact.sa_handler;
}

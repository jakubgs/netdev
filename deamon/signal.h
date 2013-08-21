#ifndef _SIGNAL_H
#define _SIGNAL_H

extern typedef void    Sigfunc(int);   /* for signal handlers */

Sigfunc * signal(int signo, Sigfunc *func);

#endif

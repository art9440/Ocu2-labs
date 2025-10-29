#include "mythread.h"
#include <sys/types.h> 
#define _GNU_SOURCE

enum thr_state {
    THR_ALIVE = 0,
    THR_EXITED = 1
};

struct mythread
{
    pid_t tid;
    void *stack;

    size_t stack_size;

    mythread_start_routine start;
    
    _Atomic int state;

    int retval;

    _Atomic int detached;
};

#include <sys/ucontext.h>
#define _GNU_SOURCE
#include "uthread.h"

#ifndef UTHREAD_MAX
#define UTHREAD_MAX 1024
#endif

#ifndef UTHREAD_STACK_SIZE
#define UTHREAD_STACK_SIZE (64 * 1024)
#endif


struct uthread {
    ucontext_t ctx;

    void *stack;

    void *(*start_routine)(void*);
    void *arg;
    void *retval;

    uthread_state_t state;

    struct  uthread *joiner;

    int freed;
};



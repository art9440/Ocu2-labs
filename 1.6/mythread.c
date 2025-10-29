#define _GNU_SOURCE
#include "mythread.h"

#include <errno.h>
#include <linux/sched.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include <linux/futex.h>
static inline int futex_wait(_Atomic int *uaddr, int val) {
    return syscall(SYS_futex, (int *)uaddr, FUTEX_WAIT, val, NULL, NULL, 0);
}

static inline int futex_wake(_Atomic int *uaddr, int n){
    return syscall(SYS_futex, (int *)uaddr, FUTEX_WAKE, n, NULL, NULL, 0);
}


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
    void *arg;
    
    _Atomic int state;

    int retval;

    _Atomic int detached;
};

static int thread_trampoline(void *arg){
    struct mythread *t = (struct mythread*)arg;

    int rc = t->start(t->arg);
    t ->retval = rc;

    atomic_store_explicit(&t->state, THR_EXITED, memory_order_release);
    futex_wake(&t->state, 1);

    syscall(SYS_exit, 0);
}

#ifndef CLONE_ARGS_DEFINED
#define CLONE_ARGS_DEFINED
#endif

static const int CLONE_FLAGS =
    CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND |
    CLONE_THREAD | CLONE_SYSVSEM;


static const size_t DEFAULT_STACK = 1 << 20;

int mythread_create(mythread_t **thr_out,
                    mythread_start_routine start_routine,
                    void *arg)
{
    if (!thr_out || !start_routine) return EINVAL;

    struct mythread *t = calloc(1, sizeof(*t));

    if (!t) return ENOMEM;

    t->stack_size = DEFAULT_STACK;

    void *stk = mmap(NULL, t->stack_size, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);

    if (stk == MAP_FAILED){
        int e = errno;
        free(t);
        return e ? e : ENOMEM;
    }

    t->stack = stk;
    t->start = start_routine;
    t->arg = arg;
     
    atomic_store(&t->state, THR_ALIVE);
    atomic_store(&t->detached, 0);

    void* child_stack_top = (char *)t->stack + t->stack_size;

    int tid = clone(thread_trampoline, child_stack_top, CLONE_FLAGS, t);

    if (tid == -1){
        int e = errno;
        munmap(t->stack, t->stack_size);
        free(t);
        return e ? e: EFAULT;
    }

    t->tid = tid;

    *thr_out = t;
    

    return 0;
}

static inline void wait_exited(_Atomic int *state){
    int s = atomic_load_explicit(state, memory_order_acquire);
    while (s != THR_EXITED){
        futex_wait(state, THR_ALIVE);
        s = atomic_load_explicit(state, memory_order_acquire);
    }
}

int mythread_join(mythread_t *t, int *ret_code_out){
    if (!t) return EINVAL;
    if (atomic_load_explicit(&t->detached, memory_order_acquire)){
        return EINVAL;
    }

    wait_exited(&t->state);
    if (ret_code_out) *ret_code_out = t->retval;

    if (t->stack) munmap(t->stack, t->stack_size);
    free(t);
    return 0;
}


void mythread_yield(void) {sched_yield();}


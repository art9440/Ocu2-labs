#include <stdio.h>
#include <stdlib.h>
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


static uthread_t *runq[UTHREAD_MAX];
static int rq_head = 0, rq_tail = 0;

static int rq_empty(void) {return rq_head == rq_tail;}
static int rq_next(int x) {return (x + 1) % UTHREAD_MAX;}

static void rq_push(uthread_t *t) {
    int nt = rq_next(rq_tail);
    if (nt == rq_head) {
        fprintf(stderr, "uthread: run queue overflow\n");
        abort();
    }
    runq[rq_tail] = t;
    rq_tail = nt; 
}

static uthread_t *rq_pop(void) {
    if (rq_empty()) return NULL;

    uthread_t *t = runq[rq_head];
    rq_head = rq_next(rq_head);

    return t;
}

static ucontext_t sched_ctx;
static ucontext_t main_ctx;
static uthread_t *current_ctx = NULL;
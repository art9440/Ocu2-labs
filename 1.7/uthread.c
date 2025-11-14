#include <stdio.h>
#include <stdlib.h>
#include <sys/ucontext.h>
#include <ucontext.h>
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
static uthread_t *current= NULL;

static void trampoline(uthread_t *t);

int uthread_create(uthread_t **out, void *(*start_routine)(void *), void *arg){
    if (!out || !start_routine) return -1;

    uthread_t *t = (uthread_t*)calloc(1, sizeof(*t));
    if (!t) return -1;

    if (getcontext(&t->ctx) == -1){
        free(t);
        return -1;
    }

    t->stack = malloc(UTHREAD_STACK_SIZE);
    if (!t->stack){
        free(t);
        return -1;
    }

    t->ctx.uc_stack.ss_sp = t ->stack;
    t->ctx.uc_stack.ss_size = UTHREAD_STACK_SIZE;
    t->ctx.uc_link = &sched_ctx;

    t->start_routine = start_routine;
    t->arg = arg;
    t->state = UTHREAD_READY;
    t->joiner = NULL;

    makecontext(&t->ctx, (void (*)(void)) trampoline, 1, t);

    rq_push(t);
    *out = t;
    return 0;
}

static void trampoline(uthread_t *t) {
    current = t;
    t->state = UTHREAD_RUNNING;

    void *ret = t->start_routine(t->arg);

    t->retval = ret;
    t->state = UTHREAD_FINISHED;

    if (t->joiner){
        if (t->joiner->state == UTHREAD_BLOCKED){
            t->joiner->state = UTHREAD_READY;
            rq_push(t->joiner);
        }
        t->joiner = NULL;
    }

    //тут после завершения улетим в планировщик, так как t->ctx.uc_link = &sched_ctx;
}
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

void uthread_yield(void){
    if (!current) return;

    current->state = UTHREAD_READY;
    rq_push(current);

    swapcontext(&current->ctx, &sched_ctx);
}


int uthread_join(uthread_t *t, void **retval) {
    if (!t) return -1;

    if (t == current) return -1;

    if (t->state != UTHREAD_FINISHED){
        if(t->joiner && t->joiner != current) {
            return -1;
        }

        t->joiner = current;
        current->state = UTHREAD_BLOCKED;

        swapcontext(&current->ctx, &sched_ctx);
    }

    if (retval) *retval = t->retval;

    uthread_destroy(t);

    return 0;
}

void uthread_destroy(uthread_t *t) {
    if (!t || t->freed) return;

    if(t->stack) free(t->stack);
    t->freed = 1;
    free(t);
}



static void schedule_work(void) {
    while(1) {
        uthread_t *next = rq_pop();
        if (!next){
            setcontext(&main_ctx);
        }
        current = next;
        next->state = UTHREAD_RUNNING;
        swapcontext(&sched_ctx, &next->ctx);
        current = NULL;
    }
}

static void schedule(void) {
    static int inited = 0;

    if (!inited) {
        getcontext(&sched_ctx);

        static char sched_stack[UTHREAD_STACK_SIZE];

        sched_ctx.uc_stack.ss_sp = sched_stack;
        sched_ctx.uc_stack.ss_size = sizeof(sched_stack);
        sched_ctx.uc_link = &main_ctx;
        makecontext(&sched_ctx, schedule_work, 0);
    }

    swapcontext(&main_ctx, &sched_ctx);
}

void uthread_run(void) {
    schedule();
}
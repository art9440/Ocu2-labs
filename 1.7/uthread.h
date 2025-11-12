#pragma once
#include <ucontext.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct uthread uthread_t;

typedef enum {
    UTHREAD_READY,
    UTHREAD_RUNNING,
    UTHREAD_BLOCKED,
    UTHREAD_FINISHED
} uthread_state_t;

int uthread_create(uthread_t **out, void *(*start_routine)(void *), void * arg);

void uthread_yield(void);

int uthread_join(uthread_t *t, void **retval);

void uthread_run(void); //запуск планировщика

void uthread_destroy(uthread_t *t); //очистка потока после завершения в join 
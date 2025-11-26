#ifndef MYTHREAD_H
#define MYTHREAD_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mythread mythread_t;
typedef int (*mythread_start_routine)(void *arg);

int mythread_create(mythread_t **thr_out,
                    mythread_start_routine start_routine,
                    void *arg);

int mythread_join(mythread_t *thr, int *ret_code_out);

int mythread_detach(mythread_t *thr);

void mythread_yield(void);




#ifdef __cplusplus
}
#endif
#endif
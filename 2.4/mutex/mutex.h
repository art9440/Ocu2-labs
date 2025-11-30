#ifndef FUTEX_MUTEX_H
#define FUTEX_MUTEX_H

#include <stdatomic.h>

typedef struct {
    atomic_int value;  // 0 - unlocked, 1 - locked/no waiters, 2 - locked/with waiters
} futex_mutex_t;

void futex_mutex_init(futex_mutex_t *m);
void futex_mutex_lock(futex_mutex_t *m);
void futex_mutex_unlock(futex_mutex_t *m);

#endif
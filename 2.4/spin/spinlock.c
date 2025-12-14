#include "spinlock.h"
#include <sched.h>  

void spinlock_init(spinlock_t *lock) {
    atomic_store(&lock->flag, 0);
}

void spinlock_lock(spinlock_t *lock) {
    int expected;
    for (;;) {
        expected = 0;
        if (atomic_compare_exchange_weak(&lock->flag, &expected, 1)) {
            return; 
        }

        sched_yield();
    }
}

void spinlock_unlock(spinlock_t *lock) {
    atomic_store(&lock->flag, 0);
}
#define _GNU_SOURCE
#include "mutex.h"

#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

static int futex_wait(atomic_int *addr, int expected) {
    int *p = (int *)addr;
    return syscall(SYS_futex, p, FUTEX_WAIT, expected, NULL, NULL, 0);
}

static int futex_wake(atomic_int *addr, int n) {
    int *p = (int *)addr;
    return syscall(SYS_futex, p, FUTEX_WAKE, n, NULL, NULL, 0);
}

void futex_mutex_init(futex_mutex_t *m) {
    atomic_store(&m->value, 0);
}

void futex_mutex_lock(futex_mutex_t *m) {
    int c = 0;

    
    if (atomic_compare_exchange_strong(&m->value, &c, 1)) {
        return;
    }

    for (;;) {
        c = atomic_load(&m->value);

        if (c == 0) {
            int expected = 0;
            if (atomic_compare_exchange_strong(&m->value, &expected, 1)) {
                return;
            }
            continue;
        }

        if (c != 2) {
            int old = atomic_exchange(&m->value, 2);
            if (old == 0) {
                return;
            }
        }

        futex_wait(&m->value, 2);
    }
}

void futex_mutex_unlock(futex_mutex_t *m) {
    int old = atomic_fetch_sub(&m->value, 1);

    if (old == 1) {
        return;
    }

    atomic_store(&m->value, 0);
    futex_wake(&m->value, 1);
}

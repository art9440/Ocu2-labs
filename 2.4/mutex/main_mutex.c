#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "mutex.h"

#define N_THREADS 4
#define N_ITERS   1000000

static long long shared_counter = 0;
static futex_mutex_t g_futex_mutex;
static pthread_mutex_t pthread_mutex;

static inline uint64_t ns_now() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

void *worker_futex(void *arg) {
    (void)arg;
    for (int i = 0; i < N_ITERS; ++i) {
        futex_mutex_lock(&g_futex_mutex);
        shared_counter++;
        futex_mutex_unlock(&g_futex_mutex);
    }
    return NULL;
}

void *worker_pthread_mutex(void *arg) {
    (void)arg;
    for (int i = 0; i < N_ITERS; ++i) {
        pthread_mutex_lock(&pthread_mutex);
        shared_counter++;
        pthread_mutex_unlock(&pthread_mutex);
    }
    return NULL;
}

static void test_sync(void *(*func)(void *), const char *test_name)
{
    pthread_t threads[N_THREADS];
    shared_counter = 0;

    uint64_t t1 = ns_now();

    for (int i = 0; i < N_THREADS; ++i) {
        pthread_create(&threads[i], NULL, func, NULL);
    }
    for (int i = 0; i < N_THREADS; ++i) {
        pthread_join(threads[i], NULL);
    }

    uint64_t t2 = ns_now();
    double dt_ms = (double)(t2 - t1) / 1e6;

    printf("Result with %s: shared_counter=%lld, expected=%lld | %.2f ms\n",
           test_name,
           shared_counter,
           (long long)N_THREADS * N_ITERS,
           dt_ms);
}

int main(void) {
    futex_mutex_init(&g_futex_mutex);
    pthread_mutex_init(&pthread_mutex, NULL);

    test_sync(worker_futex, "futex_mutex");
    test_sync(worker_pthread_mutex, "pthread_mutex");

    pthread_mutex_destroy(&pthread_mutex);
    return 0;
}

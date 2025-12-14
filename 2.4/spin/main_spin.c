#include <pthread.h>
#include <stdio.h>
#include <stdint.h>

#include "spinlock.h"

#define N_THREADS 4
#define N_ITERS   1000000


static long long shared_counter  = 0;
static spinlock_t     g_spinlock;
static pthread_spinlock_t pthread_spinlock;

static inline uint64_t ns_now() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}



void *worker_myspin(void *arg) {
    (void)arg;
    for (int i = 0; i < N_ITERS; ++i) {
        spinlock_lock(&g_spinlock);
        shared_counter++;
        spinlock_unlock(&g_spinlock);
    }
    return NULL;
}

void *worker_pthreadspin(void *arg) {
    (void)arg;
    for (int i = 0; i < N_ITERS; ++i) {
        pthread_spin_lock(&pthread_spinlock);
        shared_counter++;
        pthread_spin_unlock(&pthread_spinlock);
    }
    return NULL;
}

static void test_sync(void *(*func)(void *), const char *test_name){
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


    printf("Result with %s: shared_counter=%lld, expected=%lld | %.2f ms\n", test_name, shared_counter,  N_THREADS * (long long)N_ITERS, (t2 - t1) / 1e6);
}



int main(){
    
    pthread_spin_init(&pthread_spinlock, PTHREAD_PROCESS_PRIVATE);
    spinlock_init(&g_spinlock);

    test_sync(worker_myspin, "my_spinlock");
    test_sync(worker_pthreadspin, "pthread_spinlock");
}
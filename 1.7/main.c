#include "uthread.h"
#include <stdint.h>
#include <stdio.h>

static void work(const char *name, int n) {
    for (int i = 0; i < n; ++i) {
        printf("[%s] step %d\n", name, i);
        uthread_yield();
    }
}

static void *workerA(void *arg) {
    (void)arg;
    work("A", 3);
    return (void *)(uintptr_t)111;
}

static void *workerB(void *arg) {
    (void)arg;
    work("B", 5);
    return (void *)(uintptr_t)222;
}

int main(void) {
    uthread_t *ta = NULL, *tb = NULL;

    if (uthread_create(&ta, workerA, NULL) != 0){
        perror("uthread_create A");
        return 1;
    }

     if (uthread_create(&tb, workerB, NULL) != 0) {
        perror("uthread_create B");
        return 1;
    }

    uthread_run();

     void *retva = NULL, *retvb = NULL;
    uthread_join(ta, &retva);
    uthread_join(tb, &retvb);

    printf("join A -> %ld\n", (long)(uintptr_t)retva);
    printf("join B -> %ld\n", (long)(uintptr_t)retvb);

    return 0;
}
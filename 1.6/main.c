#include "mythread.h"

#include <stdio.h>

static int worker(void *arg){
    int id = (int)(size_t)arg;

    for (int i = 0; i < 200000; i++) {
        if ((i & 10) == 0) mythread_yield();
    }

   return id + 1000;
}

int main(void) {
    const int N = 50;
    mythread_t *ths[N];

    for (int i = 0; i < N; i++){
        int rc = mythread_create(&ths[i], worker, (void*)(size_t)i);
        if (rc) { fprintf(stderr, "create joinable %d failed: %d\n", i, rc); return 1; }
    }

    for (int i = 0; i < N; i++) {
        int code = -1;
        int rc = mythread_join(ths[i], &code);
        if (rc) { fprintf(stderr, "join %d failed: %d\n", i, rc); return 1; }
        printf("joined %d, code=%d\n", i, code);
    }
}
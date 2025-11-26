#include "threads.h"
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include "storage.h"


int main(){
    srand(time(NULL));

    Storage storage;
    init_storage(&storage);

    int n = 100;

    fill_storage_random(&storage, n);

    pthread_t asc_thread, desc_thread, equal_thread;
    pthread_t swap_thread1, swap_thread2, swap_thread3;

    pthread_create(&asc_thread, NULL, find_ascending_pairs, &storage);
    pthread_create(&desc_thread, NULL, find_descending_pairs, &storage);
    pthread_create(&equal_thread, NULL, find_equal_pairs, &storage);

    free_storage(&storage);
    pthread_mutex_destroy(&print_mutex);
}
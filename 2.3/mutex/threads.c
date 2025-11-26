#include "storage.h"
#include <pthread.h>
#include <stdio.h>
#include "threads.h"




volatile int iterations_asc = 0;
volatile int iterations_desc = 0;
volatile int iterations_equal = 0;

volatile int swap_attempts = 0;
volatile int swap_successes = 0;

pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

void* find_ascending_pairs(void* arg) {
    Storage * storage = (Storage*)arg;

    while(1) {
        int count = 0;
        Node* cur = storage->first;

        while (cur != NULL && cur->next != NULL){
            pthread_mutex_lock(&cur->sync);
            pthread_mutex_lock(&cur->next->sync);

            int len1 = string_length(cur->value);
            int len2 = string_length(cur->next->value);

            if (len1 < len2){
                count++;
            }

            pthread_mutex_unlock(&cur->next->sync);
            pthread_mutex_unlock(&cur->sync);
            
            cur = cur->next;
        }

        __sync_fetch_and_add(&iterations_asc, 1);

        pthread_mutex_lock(&print_mutex);
        printf("Ascending pairs: %d (iterations: %d)\n", count, iterations_asc);
        pthread_mutex_unlock(&print_mutex);
    }
}
#include "storage.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "threads.h"




volatile long iterations_asc = 0;
volatile long iterations_desc = 0;
volatile long iterations_equal = 0;

volatile long swap_asc = 0;
volatile long swap_desc = 0;
volatile long swap_eq = 0;

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


void* find_descending_pairs(void* arg) {
    Storage * storage = (Storage*)arg;

    while(1) {
        int count = 0;
        Node* cur = storage->first;

        while (cur != NULL && cur->next != NULL){
            pthread_mutex_lock(&cur->sync);
            pthread_mutex_lock(&cur->next->sync);

            int len1 = string_length(cur->value);
            int len2 = string_length(cur->next->value);

            if (len1 > len2){
                count++;
            }

            pthread_mutex_unlock(&cur->next->sync);
            pthread_mutex_unlock(&cur->sync);
            
            cur = cur->next;
        }

        __sync_fetch_and_add(&iterations_desc, 1);

        pthread_mutex_lock(&print_mutex);
        printf("Ascending pairs: %d (iterations: %d)\n", count, iterations_asc);
        pthread_mutex_unlock(&print_mutex);
    }
}


void* find_equal_pairs(void* arg) {
    Storage* storage = (Storage*)arg;
    
    while (1) {
        int count = 0;
        Node* current = storage->first;
        
        while (current != NULL && current->next != NULL) {
            pthread_mutex_lock(&current->sync);
            pthread_mutex_lock(&current->next->sync);
            
            int len1 = string_length(current->value);
            int len2 = string_length(current->next->value);
            
            if (len1 == len2) {
                count++;
            }
            
            pthread_mutex_unlock(&current->next->sync);
            pthread_mutex_unlock(&current->sync);
            
            current = current->next;
        }
        
        __sync_fetch_and_add(&iterations_equal, 1);
    }
}


void *thread_monitor(void *arg){
    (void)arg;

    while (1) {
        long a_it, d_it, e_it, a_sw, d_sw, e_sw;

        pthread_mutex_lock(&print_mutex);

        a_it = iterations_asc;
        d_it = iterations_desc;
        e_it = iterations_equal;

        a_sw = swap_asc;
        d_sw = swap_desc;
        e_sw = swap_eq;

        pthread_mutex_unlock(&print_mutex);

         printf("[monitor][mutex] iter: asc=%ld desc=%ld equal=%ld | "
               "swaps: asc=%ld desc=%ld equal=%ld\n",
               a_it, d_it, e_it, a_sw, d_sw, e_sw);
        fflush(stdout);
        sleep(2);
    }
}

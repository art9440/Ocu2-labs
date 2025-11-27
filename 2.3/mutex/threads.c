#include "storage.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "threads.h"




long iterations_asc = 0;
long iterations_desc = 0;
long iterations_equal = 0;

long pairs_asc   = 0; 
long pairs_desc  = 0;
long pairs_equal = 0;

long swap_asc = 0;
long swap_desc = 0;
long swap_eq = 0;


static void scan_pairs(Storage* storage,
                       int (*cmp)(int, int),
                       long* iterations_counter, long* pairs_counter)
{
    while (!storage->stop) {
        long local_pairs = 0;
        Node *sentinel = storage->first;
        Node *left = NULL;
        Node *right = NULL;

        pthread_mutex_lock(&sentinel->sync);
        left = sentinel->next;
        if (left == NULL) {
            pthread_mutex_unlock(&sentinel->sync);
            __sync_fetch_and_add(iterations_counter, 1);
            continue;
        }
        pthread_mutex_lock(&left->sync);
        pthread_mutex_unlock(&sentinel->sync);

        while (!storage->stop)
        {
            right = left->next;
            if (right == NULL) {
                break;
            }
            pthread_mutex_lock(&right->sync);

            int len1 = string_length(left->value);
            int len2 = string_length(right->value);

            if (cmp(len1, len2)) {
                local_pairs++;
            }
            pthread_mutex_unlock(&left->sync);
            left = right;
        }

        pthread_mutex_unlock(&left->sync);

        __sync_fetch_and_add(iterations_counter, 1);
        if (local_pairs > 0) {
            __sync_fetch_and_add(pairs_counter, local_pairs);
        }
    }
}

static int cmp_asc(int l1, int l2)   { return l1 < l2; }
static int cmp_desc(int l1, int l2)  { return l1 > l2; }
static int cmp_equal(int l1, int l2) { return l1 == l2; }


void* find_ascending_pairs(void* arg) {
    Storage* storage = (Storage*)arg;
    scan_pairs(storage, cmp_asc, &iterations_asc, &asc_pairs);
    return NULL;
}

void* find_descending_pairs(void* arg) {
    Storage* storage = (Storage*)arg;
    scan_pairs(storage, cmp_desc, &iterations_desc, &desc_pairs);
    return NULL;
}

void* find_equal_pairs(void* arg) {
    Storage* storage = (Storage*)arg;
    scan_pairs(storage, cmp_equal, &iterations_equal, &eq_pairs);
    return NULL;
}

static int need_swap_asc(int l1, int l2) {
    return l1 > l2;  
}

static int need_swap_desc(int l1, int l2) {
    return l1 < l2;  
}

static int need_swap_equal(int l1, int l2) {
    return l1 != l2; 
}

void do_random_swap(Storage* storage, int (*need_swap)(int, int), volatile long* swap_counter) {
    if (storage->first == NULL || storage->first->next == NULL) {
        return;
    }
    unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)pthread_self();
    int max_index = storage->count - 2;
    if (max_index < 0) return;
    
    int index = rand_r(&seed) % (max_index + 1);
    
    Node* prev = NULL;
    Node* current = storage->first;
    
    pthread_mutex_lock(&current->sync);

    for (int i = 0; i < index; i++) {
        Node* next = current->next;
        if (!next) break;
        
        pthread_mutex_lock(&next->sync);
        
        if (prev != NULL) {
            pthread_mutex_unlock(&prev->sync);
        }
        
        prev = current;
        current = next;
    }
    
    Node* next = current->next;
    if (!next) {
        
        if (prev) pthread_mutex_unlock(&prev->sync);
        pthread_mutex_unlock(&current->sync);
        return;
    }
    
    pthread_mutex_lock(&next->sync);
    
    int len1 = string_length(current->value);
    int len2 = string_length(next->value);
    
    if (need_swap(len1, len2)) {
        
        current->next = next->next;
        next->next = current;
        
        if (prev == NULL) {
           
            storage->first = next;
        } else {
            prev->next = next;
        }
        
        __sync_fetch_and_add(swap_counter, 1);
    }
    pthread_mutex_unlock(&next->sync);
    pthread_mutex_unlock(&current->sync);
    if (prev != NULL) {
        pthread_mutex_unlock(&prev->sync);
    }
}

void* swap_asc_thread(void* arg) {
    Storage* storage = (Storage*)arg;
    
    while (1) {
        do_random_swap(storage, need_swap_asc, &swap_asc);
        usleep(10000); 
    }
    return NULL;
}

// Поток для перестановки на убывание
void* swap_desc_thread(void* arg) {
    Storage* storage = (Storage*)arg;
    
    while (1) {
        do_random_swap(storage, need_swap_desc, &swap_desc);
        usleep(10000); // 10ms задержка
    }
    return NULL;
}

// Поток для перестановки на равенство
void* swap_equal_thread(void* arg) {
    Storage* storage = (Storage*)arg;
    
    while (1) {
        do_random_swap(storage, need_swap_equal, &swap_eq);
        usleep(10000); // 10ms задержка
    }
    return NULL;
}

void *thread_monitor(void *arg){
    (void)arg;

    while (1) {
        long a_it, d_it, e_it, a_sw, d_sw, e_sw;

       

        a_it = iterations_asc;
        d_it = iterations_desc;
        e_it = iterations_equal;

        a_sw = swap_asc;
        d_sw = swap_desc;
        e_sw = swap_eq;

    

         printf("[monitor][mutex] iter: asc=%ld desc=%ld equal=%ld | "
               "swaps: asc=%ld desc=%ld equal=%ld\n",
               a_it, d_it, e_it, a_sw, d_sw, e_sw);
        fflush(stdout);
        sleep(2);
    }
}

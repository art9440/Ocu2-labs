#include "storage.h"
#include <pthread.h>
#include <sched.h>
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

        pthread_spin_lock(&sentinel->sync);
        left = sentinel->next;
        if (left == NULL) {
            pthread_spin_unlock(&sentinel->sync);
            __sync_fetch_and_add(iterations_counter, 1);
            continue;
        }
        pthread_spin_lock(&left->sync);
        pthread_spin_unlock(&sentinel->sync);

        while (!storage->stop)
        {
            right = left->next;
            if (right == NULL) {
                break;
            }
            pthread_spin_lock(&right->sync);

            int len1 = string_length(left->value);
            int len2 = string_length(right->value);

            if (cmp(len1, len2)) {
                local_pairs++;
            }
            pthread_spin_unlock(&left->sync);
            left = right;

   
        }

        pthread_spin_unlock(&left->sync);

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
    scan_pairs(storage, cmp_asc, &iterations_asc, &pairs_asc);
    return NULL;
}

void* find_descending_pairs(void* arg) {
    Storage* storage = (Storage*)arg;
    scan_pairs(storage, cmp_desc, &iterations_desc, &pairs_desc);
    return NULL;
}

void* find_equal_pairs(void* arg) {
    Storage* storage = (Storage*)arg;
    scan_pairs(storage, cmp_equal, &iterations_equal, &pairs_equal);
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
    if (storage->count < 2) return;

    Node* sentinel = storage->first;
    if (!sentinel) return;

    unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)pthread_self();
    int max_pair_index = storage->count - 1;
    int pair_index = rand_r(&seed) % max_pair_index;
    
    Node* prev = sentinel;          
    pthread_spin_lock(&prev->sync);
    
    Node* current = prev->next;
    if (!current) {
        pthread_spin_unlock(&prev->sync);
        return;
    }
    pthread_spin_lock(&current->sync);

    for (int i = 0; i < pair_index; i++) {
        Node* next = current->next;
        if (!next) break;
        
        pthread_spin_lock(&next->sync);
        
        pthread_spin_unlock(&prev->sync);
        
        prev = current;
        current = next;


    }
    
    Node* next = current->next;
    if (!next) {
        pthread_spin_unlock(&current->sync);
        pthread_spin_unlock(&prev->sync);
        return;
    }
    
    pthread_spin_lock(&next->sync);
    
    int len1 = string_length(current->value);
    int len2 = string_length(next->value);
    
    if (need_swap(len1, len2)) {

        prev->next    = next;
        current->next = next->next;
        next->next    = current;

        __sync_fetch_and_add(swap_counter, 1);
    }
    pthread_spin_unlock(&next->sync);
    pthread_spin_unlock(&current->sync);
    pthread_spin_unlock(&prev->sync);


}

// Поток для перестановки на возрастание
void* swap_asc_thread(void* arg) {
    Storage* storage = (Storage*)arg;
    
    while (1) {
        do_random_swap(storage, need_swap_asc, &swap_asc);


    }
    return NULL;
}

// Поток для перестановки на убывание
void* swap_desc_thread(void* arg) {
    Storage* storage = (Storage*)arg;
    
    while (1) {
        do_random_swap(storage, need_swap_desc, &swap_desc);


    }
    return NULL;
}

// Поток для перестановки на равенство
void* swap_equal_thread(void* arg) {
    Storage* storage = (Storage*)arg;
    
    while (1) {
        do_random_swap(storage, need_swap_equal, &swap_eq);

    }
    return NULL;
}

void *thread_monitor(void *arg){
    Storage *storage = (Storage *)arg;

    while (!storage->stop) {
        long a_it = iterations_asc;
        long d_it = iterations_desc;
        long e_it = iterations_equal;

        long a_sw = swap_asc;
        long d_sw = swap_desc;
        long e_sw = swap_eq;

        long a_pairs = pairs_asc;
        long d_pairs = pairs_desc;
        long e_pairs = pairs_equal;

        printf("[monitor][spin] iter: asc=%ld desc=%ld equal=%ld | "
               "pairs: asc=%ld desc=%ld equal=%ld | "
               "swaps: asc=%ld desc=%ld equal=%ld\n",
               a_it, d_it, e_it,
               a_pairs, d_pairs, e_pairs,
               a_sw, d_sw, e_sw);
        fflush(stdout);
        sleep(2);
    }
    return NULL;
}

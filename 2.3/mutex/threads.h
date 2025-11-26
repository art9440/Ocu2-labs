#ifndef THREADS_H
#define THREADS_H

#include "storage.h"


extern volatile long iterations_asc;
extern volatile long iterations_desc;
extern volatile long iterations_equal;

extern volatile long swap_asc;
extern volatile long swap_desc;
extern volatile long swap_eq;

void* find_ascending_pairs(void* arg);
void* find_descending_pairs(void* arg);
void* find_equal_pairs(void* arg);

extern pthread_mutex_t print_mutex;

void* swap_asc_thread(void* arg);   
void* swap_desc_thread(void* arg);  
void* swap_equal_thread(void* arg); 


void do_random_swap(Storage* storage, int (*need_swap)(int, int), volatile long* swap_counter);

void *thread_monitor(void *arg);

#endif
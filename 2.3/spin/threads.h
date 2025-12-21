#ifndef THREADS_H
#define THREADS_H

#include "storage.h"

extern long iterations_asc;
extern long iterations_desc;
extern long iterations_equal;

extern long pairs_asc;
extern long pairs_desc;
extern long pairs_equal;

extern long swap_asc;
extern long swap_desc;
extern long swap_eq;

void* find_ascending_pairs(void* arg);
void* find_descending_pairs(void* arg);
void* find_equal_pairs(void* arg);

void* swap_asc_thread(void* arg);   
void* swap_desc_thread(void* arg);  
void* swap_equal_thread(void* arg); 

void do_random_swap(Storage* storage, int (*need_swap)(int, int), volatile long* swap_counter);

void *thread_monitor(void *arg);

#endif

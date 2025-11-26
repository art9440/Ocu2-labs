#ifndef THREADS_H
#define THREADS_H

#include "storage.h"


extern volatile int iterations_asc;
extern volatile int iterations_desc;
extern volatile int iterations_equal;

extern volatile int swap_attemts;
extern volatile int swap_success;

void* find_ascending_pairs(void* arg);
void* find_descending_pairs(void* arg);
void* find_equal_pairs(void* arg);
void* swap_thread(void* arg);

extern pthread_mutex_t print_mutex;

int should_swap(Node* node1, Node* node2);
#endif
#ifndef STORAGE_H
#define STORAGE_H

#include <pthread.h>

typedef struct _Node {
    char value[100];
    struct _Node* next;
    pthread_spinlock_t sync; 
} Node;


typedef struct _Storage {
    Node *first;
    Node *last;
    int count;
    volatile int stop;
} Storage;


void init_storage(Storage* storage);

Node* create_node(const char* value);

void add_to_storage(Storage* storage, const char* value);

void fill_storage_random(Storage* storage, int count);

void free_storage(Storage* storage);

void generate_random_string(char* buffer, int max_length);

int string_length(const char* str);

#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include "storage.h"

void init_storage(Storage* storage) {
    storage->first = NULL;
}

void generate_random_string(char *buffer, int max_length){
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    int length = rand() % (max_length - 1) + 1;

        for (int i = 0; i < length; i++) {
        buffer[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    buffer[length] = '\0';
}

Node* create_node(char const*  value){
    Node* new_node = (Node*)malloc(sizeof(Node));
    if (!new_node) return NULL;

    strncpy(new_node->value, value, 99);
    new_node->value[99] = '\0';
    new_node->next = NULL;
    pthread_mutex_init(&new_node->sync, NULL);

    return new_node;
}

void add_to_storage(Storage* storage, const char* value){
    Node* new_node = create_node(value);
    if (!new_node) return; 
    if (storage->first == NULL) {
        storage->first = new_node;
    }
    else{
        Node* cur = storage->first;
        while (cur->next != NULL){
            cur = cur->next;
        }

        cur->next = new_node;
    }
}


void fill_storage_random(Storage* storage, int count){

    for (int i = 0; i < count; i++){
        char random_string[100];
        generate_random_string(random_string, 100);
        add_to_storage(storage, random_string);
    }
}
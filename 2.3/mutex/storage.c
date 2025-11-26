#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
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


void fill_storage_random(Storage* storage, int count){

    for (int i = 0; i < count; i++){
        char random_string[100];
        generate_random_string(random_string, 100);

    }
}
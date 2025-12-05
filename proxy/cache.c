#include "cache.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>



static cache_entry_t cache[NUM_CACHE_ENTRIES];
static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;

void cache_init(void){
    memset(cache, 0, sizeof(cache));
}


int cache_find(const char *url) {
    int idx = -1;

    pthread_mutex_lock(&cache_mutex);
    for (int i = 0; i < NUM_CACHE_ENTRIES; i++){
        if (cache[i].valid && strcmp(cache[i].url, url) == 0) {
            cache[i].last_used = time(NULL);
            idx = i;
            break;
        }
    }
    pthread_mutex_unlock(&cache_mutex);
    return idx;
}


int cache_copy_entry_data(int idx, char **out_buf, size_t *out_size) {
    int ok = -1;

    pthread_mutex_lock(&cache_mutex);
    if (cache[idx].valid && cache[idx].data && cache[idx].size > 0) {
        *out_size = cache[idx].size;
        *out_buf = malloc(*out_size);
        if (*out_buf){
            memcpy(*out_buf, cache[idx].data, *out_size);
            ok = 0;
        }
    }
    pthread_mutex_unlock(&cache_mutex);
    return ok;
}

int cache_evict_index(void) {
    int idx = -1;

    pthread_mutex_lock(&cache_mutex);

    for (int i = 0; i < NUM_CACHE_ENTRIES; i++){
        if (!cache[i].valid && cache[i].data == NULL) {
            idx = i;
            break;
        }
    }
    if (idx == -1){
        time_t oldest = time(NULL);
        int oldest_i = 0;
        for (int i = 0; i < NUM_CACHE_ENTRIES; i++) {
            if (!cache[i].valid && cache[i].last_used <= oldest){
                oldest = cache[i].last_used;
                oldest_i = i;
            }
        }
        idx = oldest_i;
        free(cache[idx].data);
        cache[idx].data = NULL;
        cache[idx].size = 0;
        cache[idx].capacity = 0;
        cache[idx].valid = 0;
    }
    pthread_mutex_unlock(&cache_mutex);
    return idx;
}

//TODO: написать функцию для подготовки места в кэше. 
int cache_init_entry(int idx, const char *url){

}
//TODO: Написать функцию по добавлению data в кэш.
int cache_append(int idx, const char *buf, size_t n){

}

//TODO: Написать функцию по отметки блока кэша как valid, то есть его можно использовать для подгрузки из кэша
void cache_mark_valid(int idx){

}

//TODO: Написать функцию для очистки битого кэша (если появилась ошибка при загрузке в кэш)
void cache_free_unvalid(int idx){

}


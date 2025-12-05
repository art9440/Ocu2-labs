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
    pthread_mutex_lock(&cache_mutex);
    cache[idx].valid = 0;
    strncpy(cache[idx].url, url, MAX_URL_LEN - 1);
    cache[idx].url[MAX_URL_LEN - 1] = '\0';
    cache[idx].size = 0;
    cache[idx].capacity = 64 * 1024;
    cache[idx].data = malloc(cache[idx].capacity);
    if (!cache[idx].data) {
        pthread_mutex_unlock(&cache_mutex);
        return -1;
    }
    cache[idx].last_used = time(NULL);
    pthread_mutex_unlock(&cache_mutex);
    return 0;
}
//TODO: Написать функцию по добавлению data в кэш.
int cache_append(int idx, const char *buf, size_t n){
    pthread_mutex_lock(&cache_mutex);
    if (!cache[idx].data) {
        pthread_mutex_unlock(&cache_mutex);
        return -1;
    }

    if (cache[idx].size + n > CACHE_MAX_SIZE) {
        pthread_mutex_unlock(&cache_mutex);
        return -1;
    }

    if (cache[idx].size + n > cache[idx].capacity) {
        size_t newcap = cache[idx].capacity * 2;
        while (newcap < cache[idx].size + n) newcap *= 2;
        char *tmp = realloc(cache[idx].data, newcap);
        if (!tmp) {
            pthread_mutex_unlock(&cache_mutex);
            return -1;
        }
        cache[idx].data = tmp;
        cache[idx].capacity = newcap;
    }

    memcpy(cache[idx].data + cache[idx].size, buf, n);
    cache[idx].size += n;
    pthread_mutex_unlock(&cache_mutex);
    return 0;
}

//TODO: Написать функцию по отметки блока кэша как valid, то есть его можно использовать для подгрузки из кэша
void cache_mark_valid(int idx){
    pthread_mutex_lock(&cache_mutex);
    cache[idx].valid = 1;
    cache[idx].last_used = time(NULL);
    pthread_mutex_unlock(&cache_mutex);
}

//TODO: Написать функцию для очистки битого кэша (если появилась ошибка при загрузке в кэш)
void cache_free_unvalid(int idx){
    pthread_mutex_lock(&cache_mutex);
    free(cache[idx].data);
    cache[idx].data = NULL;
    cache[idx].size = 0;
    cache[idx].capacity = 0;
    cache[idx].url[0] = '\0';
    pthread_mutex_unlock(&cache_mutex);
}


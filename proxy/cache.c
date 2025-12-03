#include "cache.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>



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


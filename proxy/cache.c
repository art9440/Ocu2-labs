#include "cache.h"
#include <pthread.h>
#include <string.h>



static cache_entry_t cache[NUM_CACHE_ENTRIES];
static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;

void cache_init(void){
    memset(cache, 0, sizeof(cache));
}


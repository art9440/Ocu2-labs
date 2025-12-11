#ifndef CACHE_H
#define CACHE_H

#include <pthread.h>
#include <stddef.h>
#include <time.h>

#define MAX_URL_LEN       512
#define NUM_CACHE_ENTRIES 32

typedef struct cache_entry {
    int in_use;
    char url[MAX_URL_LEN];

    char  *data;
    size_t size;
    size_t capacity;

    int complete;
    int failed;

    int refcnt;
    time_t last_used;

    pthread_mutex_t lock;
    pthread_cond_t  cond;
} cache_entry_t;

void cache_init(void);

cache_entry_t *cache_get(const char *url, int *is_writer);

int  cache_append(cache_entry_t *e, const void *data, size_t len);

void cache_mark_valid(cache_entry_t *e);
void cache_mark_failed(cache_entry_t *e);

void cache_release(cache_entry_t *e);

#endif
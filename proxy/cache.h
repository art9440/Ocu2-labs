#ifndef CACHE_H
#define CACHE_H

#include <signal.h>
#define MAX_URL_LEN 512
#define NUM_CACHE_ENTRIES 32
#define CACHE_MAX_SIZE (1024 * 1024 * 5)

typedef struct{
    int valid;
    char url[MAX_URL_LEN];
    char *data;
    size_t size;
    size_t capacity;
    time_t last_used;
}cache_entry_t;

void cache_init(void);

int cache_find(const char *url);

int cache_copy_entry_data(int idx, char **out_buf, size_t *out_size);

int cache_evict_index(void);

int cache_init_entry(int idx, const char *url);

int cache_append(int idx, const char *buf, size_t n);

void cache_mark_valid(int idx);

void cache_free_unvalid(int idx);
#endif
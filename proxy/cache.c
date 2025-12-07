#include "cache.h"

#include <stdlib.h>
#include <string.h>

static cache_entry_t cache[NUM_CACHE_ENTRIES];
static pthread_mutex_t cache_table_mutex = PTHREAD_MUTEX_INITIALIZER;

static void cache_entry_reset(cache_entry_t *e) {
    if (!e) return;

    if (e->data) {
        free(e->data);
        e->data = NULL;
    }
    e->data      = NULL;
    e->size      = 0;
    e->capacity  = 0;
    e->complete  = 0;
    e->failed    = 0;
    e->refcnt    = 0;
    e->last_used = 0;
    e->in_use    = 0;
    e->url[0]    = '\0';
}

void cache_init(void) {
    pthread_mutex_lock(&cache_table_mutex);
    for (int i = 0; i < NUM_CACHE_ENTRIES; ++i) {
        cache_entry_t *e = &cache[i];
        memset(e, 0, sizeof(*e));
        pthread_mutex_init(&e->lock, NULL);
        pthread_cond_init(&e->cond, NULL);
        e->in_use = 0;
    }
    pthread_mutex_unlock(&cache_table_mutex);
}

static int cache_evict_lru_index(void) {
    time_t oldest = 0;
    int oldest_idx = -1;

    for (int i = 0; i < NUM_CACHE_ENTRIES; ++i) {
        cache_entry_t *e = &cache[i];
        if (!e->in_use) {
            return i;
        }
        if (e->refcnt == 0) {
            if (oldest_idx == -1 || e->last_used <= oldest) {
                oldest = e->last_used;
                oldest_idx = i;
            }
        }
    }
    return oldest_idx;
}

cache_entry_t *cache_get(const char *url, int *is_writer) {
    if (!url || !is_writer) return NULL;

    pthread_mutex_lock(&cache_table_mutex);

    for (int i = 0; i < NUM_CACHE_ENTRIES; ++i) {
        cache_entry_t *e = &cache[i];
        if (e->in_use && strcmp(e->url, url) == 0) {
            e->refcnt++;
            e->last_used = time(NULL);
            *is_writer = 0;
            pthread_mutex_unlock(&cache_table_mutex);
            return e;
        }
    }

    int idx = -1;
    for (int i = 0; i < NUM_CACHE_ENTRIES; ++i) {
        if (!cache[i].in_use) {
            idx = i;
            break;
        }
    }

    if (idx == -1) {
        idx = cache_evict_lru_index();
        if (idx == -1) {
            pthread_mutex_unlock(&cache_table_mutex);
            return NULL;
        }
        cache_entry_reset(&cache[idx]);
    }

    cache_entry_t *e = &cache[idx];
    cache_entry_reset(e);
    e->in_use = 1;
    strncpy(e->url, url, MAX_URL_LEN - 1);
    e->url[MAX_URL_LEN - 1] = '\0';
    e->data      = NULL;
    e->size      = 0;
    e->capacity  = 0;
    e->complete  = 0;
    e->failed    = 0;
    e->refcnt    = 1;
    e->last_used = time(NULL);

    *is_writer = 1;
    pthread_mutex_unlock(&cache_table_mutex);
    return e;
}

int cache_append(cache_entry_t *e, const void *data, size_t len) {
    if (!e || !data || len == 0) return 0;

    pthread_mutex_lock(&e->lock);

    if (e->failed) {
        pthread_mutex_unlock(&e->lock);
        return -1;
    }

    size_t need = e->size + len;
    if (need > e->capacity) {
        size_t newcap = e->capacity ? e->capacity * 2 : 64 * 1024;
        if (newcap < need)
            newcap = need;

        char *newdata = realloc(e->data, newcap);
        if (!newdata) {
            e->failed = 1;
            pthread_cond_broadcast(&e->cond);
            pthread_mutex_unlock(&e->lock);
            return -1;
        }
        e->data     = newdata;
        e->capacity = newcap;
    }

    memcpy(e->data + e->size, data, len);
    e->size += len;
    e->last_used = time(NULL);

    pthread_cond_broadcast(&e->cond);
    pthread_mutex_unlock(&e->lock);

    return 0;
}

void cache_mark_valid(cache_entry_t *e) {
    if (!e) return;
    pthread_mutex_lock(&e->lock);
    e->complete = 1;
    e->last_used = time(NULL);
    pthread_cond_broadcast(&e->cond);
    pthread_mutex_unlock(&e->lock);
}

void cache_mark_failed(cache_entry_t *e) {
    if (!e) return;
    pthread_mutex_lock(&e->lock);
    e->failed = 1;
    pthread_cond_broadcast(&e->cond);
    pthread_mutex_unlock(&e->lock);
}

void cache_release(cache_entry_t *e) {
    if (!e) return;

    pthread_mutex_lock(&cache_table_mutex);
    if (e->refcnt > 0)
        e->refcnt--;
    pthread_mutex_unlock(&cache_table_mutex);
}
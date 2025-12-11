#define _GNU_SOURCE
#include "logger.h"

#include <stdarg.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

static FILE *log_file = NULL;
static log_level_t log_min_level = LOG_DEBUG;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

int log_init(const char *path) {
    pthread_mutex_lock(&log_mutex);

    if (log_file && log_file != stderr) {
        fclose(log_file);
        log_file = NULL;
    }

    if (path == NULL) {
        log_file = stderr;
    } else {
        log_file = fopen(path, "a");
        if (!log_file) {
            log_file = stderr;
            pthread_mutex_unlock(&log_mutex);
            return -1;
        }
    }

    pthread_mutex_unlock(&log_mutex);
    return 0;
}

void log_set_level(log_level_t level) {
    pthread_mutex_lock(&log_mutex);
    log_min_level = level;
    pthread_mutex_unlock(&log_mutex);
}

static const char *level_to_str(log_level_t level) {
    switch (level) {
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO:  return "INFO ";
        case LOG_WARN:  return "WARN ";
        case LOG_ERROR: return "ERROR";
        default:        return "UNKWN";
    }
}

void log_log(log_level_t level, const char *fmt, ...) {
    if (level < log_min_level) return;

    pthread_mutex_lock(&log_mutex);

    if (!log_file) {
        log_file = stderr;
    }

    time_t now = time(NULL);
    struct tm tm_now;
    localtime_r(&now, &tm_now);

    char timebuf[32];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &tm_now);

    unsigned long tid = (unsigned long)pthread_self();

    fprintf(log_file, "[%s] [%s] [tid=%lu] ",
            timebuf, level_to_str(level), tid);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(log_file, fmt, ap);
    va_end(ap);

    fputc('\n', log_file);
    fflush(log_file);

    pthread_mutex_unlock(&log_mutex);
}

#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO  = 1,
    LOG_WARN  = 2,
    LOG_ERROR = 3
} log_level_t;

int log_init(const char *path);

void log_set_level(log_level_t level);

void log_log(log_level_t level, const char *fmt, ...);

#define LOG_DEBUGF(...) log_log(LOG_DEBUG, __VA_ARGS__)
#define LOG_INFOF(...)  log_log(LOG_INFO,  __VA_ARGS__)
#define LOG_WARNF(...)  log_log(LOG_WARN,  __VA_ARGS__)
#define LOG_ERRORF(...) log_log(LOG_ERROR, __VA_ARGS__)

#endif // LOGGER_H

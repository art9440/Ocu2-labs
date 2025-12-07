#include "proxy.h"
#include "cache.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#define BUF_SIZE 4096

static int parse_url(const char *url, char *host, size_t hlen, int *port, char *path, size_t plen)
{
    const char *p = url;
    const char *host_start;
    const char *path_start;
    *port = 80;

    if (strncmp(p, "http://", 7) == 0) {
        p += 7;
    }
    host_start = p;
    path_start = strchr(p, '/');
    if (!path_start) {
        path_start = p + strlen(p);
        strncpy(path, "/", plen);
        path[plen - 1] = '\0';
    } else {
        snprintf(path, plen, "%s", path_start);
    }

    char hostport[256];
    size_t hostlen = (size_t)(path_start - host_start);
    if (hostlen >= sizeof(hostport)) hostlen = sizeof(hostport) - 1;
    memcpy(hostport, host_start, hostlen);
    hostport[hostlen] = '\0';

    char *colon = strchr(hostport, ':');
    if (colon) {
        *colon = '\0';
        *port = atoi(colon + 1);
        if (*port <= 0 || *port > 65535) *port = 80;
    }

    strncpy(host, hostport, hlen - 1);
    host[hlen - 1] = '\0';
    return 0;
}

static ssize_t send_all(int sock, const void *buf, size_t len)
{
    const char *p = buf;
    size_t left = len;
    while (left > 0) {
        ssize_t n = send(sock, p, left, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) break;
        p += n;
        left -= (size_t)n;
    }
    return (ssize_t)(len - left);
}

static int parse_request_line(const char *line, char *method, char *url)
{
    char version[32];
    if (sscanf(line, "%15s %511s %31s", method, url, version) != 3) {
        return -1;
    }
    if (strncmp(method, "GET", 3) != 0) {
        return -1;
    }
    return 0;
}

static int connect_to_host(const char *host, int port)
{
    char port_str[16];
    struct addrinfo hints, *res, *rp;
    int sock = -1;

    snprintf(port_str, sizeof(port_str), "%d", port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port_str, &hints, &res) != 0) {
        return -1;
    }

    for (rp = res; rp != NULL; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock == -1) continue;
        if (connect(sock, rp->ai_addr, rp->ai_addrlen) == 0) break;
        close(sock);
        sock = -1;
    }

    freeaddrinfo(res);
    return sock;
}

static void stream_from_cache(cache_entry_t *entry, int client_sock)
{
    size_t offset = 0;
    char tmp[BUF_SIZE];

    for (;;) {
        pthread_mutex_lock(&entry->lock);

        while (offset == entry->size && !entry->complete && !entry->failed) {
            pthread_cond_wait(&entry->cond, &entry->lock);
        }

        if (entry->failed) {
            pthread_mutex_unlock(&entry->lock);
            LOG_ERRORF("stream_from_cache: entry for url='%s' failed, stop streaming", entry->url);
            break;
        }

        if (offset == entry->size && entry->complete) {
            pthread_mutex_unlock(&entry->lock);
            LOG_DEBUGF("stream_from_cache: finished for fd=%d, url='%s'", client_sock, entry->url);
            break;
        }

        size_t avail = entry->size - offset;
        size_t chunk = (avail < sizeof(tmp)) ? avail : sizeof(tmp);
        memcpy(tmp, entry->data + offset, chunk);
        offset += chunk;

        pthread_mutex_unlock(&entry->lock);

        if (send_all(client_sock, tmp, chunk) < 0) {
            if (errno == EPIPE || errno == ECONNRESET) {
                LOG_INFOF("stream_from_cache: client fd=%d closed connection", client_sock);
            } else {
                LOG_ERRORF("stream_from_cache: send_all to client fd=%d failed: %s",
                           client_sock, strerror(errno));
            }
            break;
        }
    }
}

static void handle_no_cache(int client_sock, const char *url, struct timespec t_start)
{
    char buf[BUF_SIZE];
    ssize_t n;

    char host[256];
    char path[512];
    int port;

    if (parse_url(url, host, sizeof(host), &port, path, sizeof(path)) < 0) {
        LOG_WARNF("fd=%d: failed to parse URL (no-cache path): %s", client_sock, url);
        return;
    }

    int origin = connect_to_host(host, port);
    if (origin < 0) {
        LOG_ERRORF("fd=%d: failed to connect to origin %s:%d (no-cache path)",
                   client_sock, host, port);
        return;
    }

    char req[1024];
    int req_len = snprintf(req, sizeof(req),
                           "GET %s HTTP/1.0\r\n"
                           "Host: %s\r\n"
                           "Connection: close\r\n"
                           "\r\n",
                           path, host);
    if (req_len <= 0 || req_len >= (int)sizeof(req)) {
        LOG_WARNF("fd=%d: failed to build request (no-cache path) for %s", client_sock, url);
        close(origin);
        return;
    }

    if (send_all(origin, req, (size_t)req_len) < 0) {
        LOG_WARNF("fd=%d: failed to send request to origin %s:%d (no-cache path)",
                  client_sock, host, port);
        close(origin);
        return;
    }

    size_t total_bytes = 0;
    while ((n = recv(origin, buf, sizeof(buf), 0)) > 0) {
        if (send_all(client_sock, buf, (size_t)n) < 0) break;
        total_bytes += (size_t)n;
    }

    close(origin);

    struct timespec t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_end);
    double ms = (t_end.tv_sec - t_start.tv_sec) * 1000.0 +
                (t_end.tv_nsec - t_start.tv_nsec) / 1e6;
    LOG_INFOF("fd=%d: finished (no-cache) %s, bytes=%zu, time=%.2f ms",
              client_sock, url, total_bytes, ms);
}

void handle_client_socket(int client_sock)
{
    char buf[BUF_SIZE];
    ssize_t n;

    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    int pos = 0;
    while (pos < (int)sizeof(buf) - 1) {
        n = recv(client_sock, buf + pos, 1, 0);
        if (n <= 0) {
            LOG_DEBUGF("fd=%d: recv() failed while reading request line (n=%zd, errno=%d)",
                       client_sock, n, errno);
            return;
        }
        if (buf[pos] == '\n') break;
        pos += 1;
    }
    buf[pos + 1] = '\0';

    char method[16];
    char url[MAX_URL_LEN];
    if (parse_request_line(buf, method, url) < 0) {
        LOG_WARNF("fd=%d: bad request line: %s", client_sock, buf);
        return;
    }

    LOG_INFOF("fd=%d: request: %s %s", client_sock, method, url);

    char line[BUF_SIZE];
    while (1) {
        int i = 0;
        char c;
        while (i < (int)sizeof(line) - 1) {
            n = recv(client_sock, &c, 1, 0);
            if (n <= 0) break;
            line[i++] = c;
            if (c == '\n') break;
        }
        if (n <= 0) break;
        line[i] = '\0';
        if (strcmp(line, "\r\n") == 0 || strcmp(line, "\n") == 0) break;
    }

    int is_writer = 0;
    cache_entry_t *entry = cache_get(url, &is_writer);
    if (!entry) {
        LOG_WARNF("fd=%d: cache disabled for %s (no free slots)", client_sock, url);
        handle_no_cache(client_sock, url, t_start);
        return;
    }

    LOG_DEBUGF("fd=%d: cache_get url='%s', is_writer=%d", client_sock, url, is_writer);

    if (!is_writer) {
        LOG_INFOF("fd=%d: cache READER for %s", client_sock, url);
        stream_from_cache(entry, client_sock);
        cache_release(entry);

        clock_gettime(CLOCK_MONOTONIC, &t_end);
        double ms = (t_end.tv_sec - t_start.tv_sec) * 1000.0 +
                    (t_end.tv_nsec - t_start.tv_nsec) / 1e6;
        LOG_INFOF("fd=%d: finished (cache reader) %s in %.2f ms", client_sock, url, ms);
        return;
    }

    LOG_INFOF("fd=%d: cache WRITER for %s", client_sock, url);

    char host[256];
    char path[512];
    int port;

    if (parse_url(url, host, sizeof(host), &port, path, sizeof(path)) < 0) {
        LOG_WARNF("fd=%d: failed to parse URL: %s", client_sock, url);
        cache_mark_failed(entry);
        cache_release(entry);
        return;
    }

    LOG_INFOF("fd=%d: forwarding to origin %s:%d path=%s", client_sock, host, port, path);

    int origin = connect_to_host(host, port);
    if (origin < 0) {
        LOG_ERRORF("fd=%d: failed to connect to origin %s:%d", client_sock, host, port);
        cache_mark_failed(entry);
        cache_release(entry);
        return;
    }

    char req[1024];
    int req_len = snprintf(req, sizeof(req),
                           "GET %s HTTP/1.0\r\n"
                           "Host: %s\r\n"
                           "Connection: close\r\n"
                           "\r\n",
                           path, host);
    if (req_len <= 0 || req_len >= (int)sizeof(req)) {
        LOG_WARNF("fd=%d: failed to build request to origin for %s", client_sock, url);
        cache_mark_failed(entry);
        cache_release(entry);
        close(origin);
        return;
    }

    if (send_all(origin, req, (size_t)req_len) < 0) {
        LOG_WARNF("fd=%d: failed to send request to origin %s:%d", client_sock, host, port);
        cache_mark_failed(entry);
        cache_release(entry);
        close(origin);
        return;
    }

    char obuf[BUF_SIZE];
    int client_alive = 1;
    size_t total_bytes = 0;

    while ((n = recv(origin, obuf, sizeof(obuf), 0)) > 0) {
        if (cache_append(entry, obuf, (size_t)n) != 0) {
            LOG_WARNF("fd=%d: cache_append failed for %s, marking failed", client_sock, url);
            cache_mark_failed(entry);
            break;
        }

        if (client_alive) {
            if (send_all(client_sock, obuf, (size_t)n) < 0) {
                if (errno == EPIPE || errno == ECONNRESET) {
                    LOG_INFOF("fd=%d: client closed while streaming %s, continue caching",
                              client_sock, url);
                    client_alive = 0;
                } else {
                    LOG_WARNF("fd=%d: send_all() failed while streaming %s (errno=%d)",
                              client_sock, url, errno);
                    client_alive = 0;
                }
            }
        }

        total_bytes += (size_t)n;
    }

    if (n == 0 && !entry->failed) {
        LOG_DEBUGF("fd=%d: origin closed, marking complete for %s", client_sock, url);
        cache_mark_valid(entry);
    } else if (n < 0 && !entry->failed) {
        LOG_WARNF("fd=%d: recv() from origin failed for %s: errno=%d", client_sock, url, errno);
        cache_mark_failed(entry);
    }

    close(origin);
    cache_release(entry);

    clock_gettime(CLOCK_MONOTONIC, &t_end);
    double ms = (t_end.tv_sec - t_start.tv_sec) * 1000.0 +
                (t_end.tv_nsec - t_start.tv_nsec) / 1e6;
    LOG_INFOF("fd=%d: finished (origin writer) %s, bytes=%zu, time=%.2f ms",
              client_sock, url, total_bytes, ms);
}
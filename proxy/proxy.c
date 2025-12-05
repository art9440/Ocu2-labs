#include "proxy.h"
#include "cache.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <time.h>
#include <unistd.h>


#define BUF_SIZE 4096

static int parse_url(const char *url, char *host, size_t hlen, int *port, char *path, size_t plen){
    const char *p = url;
    const char *host_start;
    const char *path_start;
    *port = 80;

    if (strncmp(p, "http://", 7) == 0) {
        p += 7;
    }

    host_start = p;
    path_start = strchr(p, '/');
    if (!path_start){
        path_start = p + strlen(p);
        strncpy(path, "/", plen);
        path[plen - 1] = '\0';
    }else{
        snprintf(path, plen, "%s", path_start);
    }

    char hostport[256];
    size_t hostlen = (size_t)(path_start - host_start);
    if (hostlen >= sizeof(hostport)) hostlen = sizeof(hostport) - 1;
    memcpy(hostport, host_start, hostlen);
    hostport[hostlen] = '\0';

    char *colon = strchr(hostport, ':');
    if (colon){
        *colon = '\0';
        *port = atoi(colon + 1);
        if (*port <= 0 || *port > 65535) *port = 80;
    }

    strncpy(host, hostport, hlen - 1);
    host[hlen - 1] = '\0';
    return 0;
}

static ssize_t send_all(int sock, const void *buf, size_t len){
    const char *p = buf;
    size_t left = len;
    while (left > 0) {
        ssize_t n = send(sock, p, left, 0);
        if (n < 0){
            if (errno == EINTR) continue;
            return -1;
        }

        if (n == 0) break;
        p += n;
        left -= n;
    }

     return (ssize_t)(len - left);
}

static int parse_request_line(const char *line,char *method,char *url){
    char version[32];
    if (sscanf(line, "%s %s %31s", method, url, version) != 3) {
        return -1;
    }
    if (strncmp(method, "GET", 3) != 0) {
        return -1;
    }
    
    return 0;
}

static int connect_to_host(const char *host, int port) {
    char port_str[16];
    struct addrinfo hints, *res, *rp;

    int sock = -1;

    snprintf(port_str, sizeof(port_str), "%d", port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port_str, &hints, &res) != 0){
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

void handle_client_socket(int client_sock) {
    char buf[BUF_SIZE];

    ssize_t n;

    int pos = 0;
    while(pos < (int)sizeof(buf) - 1) {
        n = recv(client_sock, buf + pos, 1, 0);
        if (n <= 0) return;

        if (buf[pos] == '\n') break;
        pos += 1;
    }

    buf[pos + 1] = '\0';

    char method[16];
    char url[MAX_URL_LEN];

    if (parse_request_line(buf, method, url) < 0) {
        fprintf(stderr, "Bad request line: %s\n", buf);
        return;
    }

    char line[BUF_SIZE];
    while (1) {
        int i = 0;
        char c;
        while (i < (int)sizeof(line) - 1){
            n = recv(client_sock, &c, 1, 0);
             if (n <= 0) break;
            line[i++] = c;
            if (c == '\n') break;
        }

        if (n <= 0) break;
        line[i] = '\0';
        if (strcmp(line, "\r\n") == 0 || strcmp(line, "\n") == 0) break;
    }


    int idx = cache_find(url);
    
    if (idx >= 0){
        char *data = NULL;
        size_t size = 0;
        if (cache_copy_entry_data(idx, &data, &size) == 0 && data) {
            send_all(client_sock, data, size);
            free(data);
        }
        return;
    }


    //TODO: если в кэше нет, то отправляем запрос к origin-серверу
    //далее, получая данные с origin-сервера одновременно записывать их в кэш и отправлять клиенту
    //Даже если во время записи в кэш произойдет ошибка, продолжить отправлять данные клиенту

    char host[256];
    char path[512];
    int port;

    if (parse_url(url, host, sizeof(host), &port, path, sizeof(path)) < 0) {
        fprintf(stderr, "Failed to parse URL: %s\n", url);
        return;
    }

    int origin = connect_to_host(host, port);
    if (origin < 0) {
        fprintf(stderr, "Failed to connect to origin: %s:%d\n", host, port);
        return;
    }

    char req[1024];

    int req_len = snprintf(req, sizeof(req), 
                    "GET %s HTTP/1.0\r\n"
                           "Host: %s\r\n"
                           "Connection: close\r\n"
                           "\r\n",
                           path, host);
    send_all(origin, req, (size_t) req_len);

    idx = cache_evict_index();
    if (cache_init_entry(idx, url) < 0) {
        fprintf(stderr, "Cache init failed, streaming without cache \n");
        while ((n = recv(origin, buf, sizeof(buf), 0)) > 0) {
            if (send_all(client_sock, buf, (size_t)n) < 0) break;
        }
        close(origin);
        close(client_sock);
        return;
    }

    int cache_ok = 1;

    while ((n = recv(origin, buf, sizeof(buf), 0)) > 0) {
        if (cache_ok){
            if (cache_append(idx, buf, (size_t)n) < 0) {
            cache_ok = 0;
            }
        }

        if (send_all(client_sock, buf, (size_t)n) < 0) {
        break;
        }
    }

    if (cache_ok) {
    cache_mark_valid(idx);
    } else {
        cache_free_unvalid(idx);
    }
}
#include "proxy.h"
#include "cache.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>


#define BUF_SIZE 4096

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

    //TODO: попытаться достать из кэша

    //TODO: если в кэше нет, то отправляем запрос к origin-серверу
    //далее, получая данные с origin-сервера одновременно записывать их в кэш и отправлять клиенту
    //Даже если во время записи в кэш произойдет ошибка, продолжить отправлять данные клиенту
}
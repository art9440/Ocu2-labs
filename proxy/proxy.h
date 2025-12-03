#ifndef PROXY_H
#define PROXY_H

#include <netinet/in.h>

typedef struct {
    int client_sock;
    struct sockaddr_in client_addr;
} client_info_t;

#endif
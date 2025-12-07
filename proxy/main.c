#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#include "cache.h"
#include "thread_pool.h"
#include "logger.h"

#define LISTEN_PORT 8080
#define BACKLOG 64
#define NUM_WORKERS 10

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(void) {
    signal(SIGPIPE, SIG_IGN);

    cache_init();
    thread_pool_init(NUM_WORKERS);

    int listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket < 0) {
        die("socket");
    }

    printf("listen_socket = %d\n", listen_socket);

    int opt = 1;
    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR,
                   &opt, sizeof(opt)) < 0) {
        die("setsockopt");
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(LISTEN_PORT);

    if (bind(listen_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        die("bind");
    }

    if (listen(listen_socket, BACKLOG) < 0) {
        die("listen");
    }

    LOG_INFOF("Proxy listening on port %d", LISTEN_PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t clen = sizeof(client_addr);
        int client_socket = accept(listen_socket,
                                   (struct sockaddr*)&client_addr, &clen);
        if (client_socket < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            continue;
        }

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
        int cport = ntohs(client_addr.sin_port);

        LOG_INFOF("New connection fd=%d from %s:%d", client_socket, ip, cport);

        thread_pool_add_client(client_socket);
    }

    close(listen_socket);
    return 0;
}
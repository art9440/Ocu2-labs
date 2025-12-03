#include <asm-generic/socket.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include "cache.h"
#include "proxy.h"

#include <pthread.h>


#define LISTEN_PORT 80
#define BACKLOG 64

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}


int main(void) {
    signal(SIGPIPE, SIG_IGN);

    cache_init();

    int listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket < 0) die("socket");

    int opt = 1;
    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
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

    printf("Proxy listening on port %d\n", LISTEN_PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t clen = sizeof(client_addr);
        int client_socket = accept(listen_socket,(struct sockaddr*)&client_addr, &clen);
        if (client_socket < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            continue;
        }

        client_info_t *ci = (client_info_t *)malloc(sizeof(client_info_t));

         if (!ci) {
            close(client_socket);
            continue;
        }
        ci->client_sock = client_socket;
        ci->client_addr = client_addr;

        pthread_t tid;

        if (pthread_create(&tid, NULL, handle_client, ci) != 0) {
            perror("pthread_create");
            close(client_socket);
            free(ci);
            continue;
        }
        pthread_detach(tid);
    }

    close(listen_socket);
    return 0;
}
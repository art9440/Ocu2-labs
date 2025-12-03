
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "thread_pool.h"
#include "proxy.h"
#define QUEUE_SIZE 128


typedef struct {
    int sockets[QUEUE_SIZE];
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} client_queue_t;

static client_queue_t queue;
static int workers_started = 0;


static void queue_init(client_queue_t *q) {
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
}

static void queue_push(client_queue_t *q, int client_sock) {
    pthread_mutex_lock(&q->mutex);
    while (q->count == QUEUE_SIZE) {
        pthread_cond_wait(&q->not_full, &q->mutex);
    }
    q->sockets[q->tail] = client_sock;
    q->tail = (q->tail + 1) % QUEUE_SIZE;
    q->count++;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->mutex);
}

static int queue_pop(client_queue_t *q) {
    pthread_mutex_lock(&q->mutex);
    while (q->count == QUEUE_SIZE){
        pthread_cond_wait(&q->not_empty, &q->mutex);
    }
    int sock = q->sockets[q->head];
    q->head = (q->head + 1) % QUEUE_SIZE;
    q->count--;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->mutex);
    return sock;
}


static void *worker_thread(void *arg) {
    (void)arg;
    for (;;) {
        int client_sock = queue_pop(&queue);
        handle_client_socket(client_sock);
        close(client_sock);
    }
    return NULL;
}



void thread_pool_init(int num_workers){
    if (workers_started) return;
    workers_started = 1;

    queue_init(&queue);

    for (int i = 0; i < num_workers; i++){
        pthread_t tid;
        if (pthread_create(&tid, NULL, worker_thread, NULL) != 0) {
            perror("pthread create in threadpool");
        }else{
            pthread_detach(tid);
        }
    }
}

void thread_pool_add_client(int client_sock) {
    queue_push(&queue, client_sock);
}
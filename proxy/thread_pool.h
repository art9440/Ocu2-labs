#ifndef THREAD_POOL_H
#define THREAD_POOL_H


void thread_pool_init(int num_workers);

void thread_pool_add_client(int client_sock);

#endif 
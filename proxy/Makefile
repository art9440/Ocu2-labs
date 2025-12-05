CC = gcc
CFLAGS = -Wall -Wextra -pthread -g

OBJS = main.o proxy.o cache.o thread_pool.o

all: proxy

proxy: $(OBJS)
	$(CC) $(CFLAGS) -o proxy $(OBJS)

main.o: main.c proxy.h cache.h
	$(CC) $(CFLAGS) -c main.c

proxy.o: proxy.c proxy.h cache.h
	$(CC) $(CFLAGS) -c proxy.c

cache.o: cache.c cache.h
	$(CC) $(CFLAGS) -c cache.c

thread_pool.o: thread_pool.c thread_pool.h
	$(CC) $(CFLAGS) -c thread_pool.c

clean:
	rm -f proxy $(OBJS)
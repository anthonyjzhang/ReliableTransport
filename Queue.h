#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

typedef struct SafeQueue
{
    size_t head, tail, count, size;
    void **queue;
    pthread_mutex_t mtx;
    pthread_cond_t empty, full;
} SafeQueue;

void queueInit(SafeQueue *sq, size_t size);
void queueClear(SafeQueue *sq);
void queueLock(SafeQueue *sq);
void queueUnlock(SafeQueue *sq);
void *queueFront(const SafeQueue *sq);
int queuePush(SafeQueue *sq, void *el);
int queuePop(SafeQueue *sq);
int queuePut(SafeQueue *sq, void *el, int timeout);  
int queuePutUnblock(SafeQueue *sq, void *el);  
void *queueGet(SafeQueue *sq, int timeout); 
void *queueGetUnblock(SafeQueue *sq);

#define DEFAULT_CAPACITY 13

typedef struct Heap
{
    size_t size, count;
    void **array;
    int (*cmp)(const void *, const void *);
} Heap;

void heapInit(Heap *h, int (*cmp)(const void *, const void *));
void heapClear(Heap *h);
size_t heapPushup(Heap *h, size_t idx);
size_t heapPushdown(Heap *h, size_t idx);
int heapPush(Heap *h, void *el);
void *heapTop(Heap *h);
int heapPop(Heap *h);

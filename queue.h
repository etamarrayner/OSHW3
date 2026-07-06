#include <stdbool.h>
#include <netinet/in.h>
#include <stdlib.h>
#include "request.h"

typedef struct {
    int fd;
    time_stats stats;
} item;

typedef struct {
    item *items;
    int MAX_SIZE;
    int front;
    int rear;
} CircularQueue;

void initQueue(CircularQueue *q, int MAX_SIZE);

bool isFull(CircularQueue *q);

bool isEmpty(CircularQueue *q);

void enqueue(CircularQueue *q, item value);

item dequeue(CircularQueue *q);

void deleteQueue(CircularQueue *q);

struct UDP_req{
    struct sockaddr_in sock_in;
    struct UDP_req *next;
};

typedef struct{
    struct UDP_req *first;
    struct UDP_req *last;
} UDP_queue;

void InitUDP_queue(UDP_queue *q);

bool isUDP_queueEmpty(UDP_queue *q);

void enqueueUDP(UDP_queue *q, struct sockaddr_in sock_in);

struct sockaddr_in dequeueUDP(UDP_queue *q);

void deleteUDP_queue(UDP_queue *q);
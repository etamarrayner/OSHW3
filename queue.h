#include <stdbool.h>
#define MAX_SIZE 10

typedef struct {
    int fd;
    int arrival_time;
} item;

typedef struct {
    item items[MAX_SIZE];
    int front;
    int rear;
} CircularQueue;

void initQueue(CircularQueue *q);

bool isFull(CircularQueue *q);

bool isEmpty(CircularQueue *q);

void enqueue(CircularQueue *q, item value);

item dequeue(CircularQueue *q);


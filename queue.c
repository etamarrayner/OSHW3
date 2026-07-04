#include "queue.h"

void initQueue(CircularQueue *q) {
    q->front = -1;
    q->rear = -1;
}

bool isFull(CircularQueue *q) {
    return ((q->rear + 1) % MAX_SIZE == q->front);
}

bool isEmpty(CircularQueue *q) {
    return (q->front == -1);
}

void enqueue(CircularQueue *q, item value) {
    if (isFull(q)) {
        printf("Queue is full, cannot enqueue %d\n", value);
        return;
    }
    if (isEmpty(q)) {
        q->front = 0;
    }
    q->rear = (q->rear + 1) % MAX_SIZE;
    q->items[q->rear] = value;
    printf("Enqueued: %d\n", value);
}

item dequeue(CircularQueue *q) {
    if (isEmpty(q)) {
        printf("Queue is empty. Cannot dequeue\n");
        return (item){-1,0};
    }
    item data = q->items[q->front];
    
    // Reset queue if it becomes empty
    if (q->front == q->rear) {
        q->front = -1;
        q->rear = -1;
    } else {
        q->front = (q->front + 1) % MAX_SIZE;
    }
    return data;
}

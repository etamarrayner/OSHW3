#include "queue.h"

void initQueue(CircularQueue *q, int MAX_SIZE) {
    q->MAX_SIZE = MAX_SIZE;
    q->front = -1;
    q->rear = -1;
    q->items = malloc(sizeof(item)*MAX_SIZE);
}

bool isFull(CircularQueue *q) {
    return ((q->rear + 1) % q->MAX_SIZE == q->front);
}

bool isEmpty(CircularQueue *q) {
    return (q->front == -1);
}

void enqueue(CircularQueue *q, item value) {
    if (isFull(q)) {
        return;
    }
    if (isEmpty(q)) {
        q->front = 0;
    }
    q->rear = (q->rear + 1) % q->MAX_SIZE;
    q->items[q->rear] = value;
}

item dequeue(CircularQueue *q) {
    if (isEmpty(q)) {
        return (item){-1,{0}};
    }
    item data = q->items[q->front];
    
    // Reset queue if it becomes empty
    if (q->front == q->rear) {
        q->front = -1;
        q->rear = -1;
    } else {
        q->front = (q->front + 1) % q->MAX_SIZE;
    }
    return data;
}

void deleteQueue(CircularQueue *q){
    free(q->items);
}

void InitUDP_queue(UDP_queue *q){
    q->first = NULL;
    q->last = NULL;
}

bool isUDP_queueEmpty(UDP_queue *q){
    return q->first == NULL;
}

void enqueueUDP(UDP_queue *q, struct sockaddr_in sock_in){
    struct UDP_req *req = malloc(sizeof(struct UDP_req));
    req->sock_in = sock_in;
    req->next = NULL;
    if(isUDP_queueEmpty(q)){
        q->last = req;
        q->first = req;
        return;
    }
    q->last->next = req;
    q->last = req;
}

struct sockaddr_in dequeueUDP(UDP_queue *q){
    if(isUDP_queueEmpty(q)){
        return (struct sockaddr_in){0};
    }
    struct UDP_req *temp_req = q->first;
    struct sockaddr_in temp_sock = temp_req->sock_in;
    q->first = temp_req->next;
    if(q->first == NULL) q->last = NULL;
    free(temp_req);
    return temp_sock;
}

void deleteUDP_queue(UDP_queue *q){
    while(!isUDP_queueEmpty(q)) dequeueUDP(q);
}
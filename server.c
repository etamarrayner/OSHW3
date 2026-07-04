#include "segel.h"
#include "request.h"
#include "log.h"
#include "queue.h"

//
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

#define NUM_WORKERS 10

//shared variables
CircularQueue connections_queue;
server_log global_log;

pthread_mutex_t lock;
pthread_cond_t cond_not_empty;
pthread_cond_t cond_not_full;


// Parses command-line arguments
void getargs(int *port, int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    *port = atoi(argv[1]);
}

// TODO: HW3 — Task 1: Initialize the thread pool and request queue.
// This server currently handles all requests in the main thread.

void* worker_function(void* arg){
    long id = (long)arg;

    while(1){
        pthread_mutex_lock(&lock);

        while(isEmpty(&connections_queue)){
            pthread_cond_wait(&cond_not_empty, &lock);
        }


        int connfd = dequeue(&connections_queue).fd;

        pthread_cond_signal(&cond_not_full);

        pthread_mutex_unlock(&lock);

        threads_stats t = malloc(sizeof(struct Threads_stats));
        t->id = 0;             // Thread ID (placeholder)
        t->stat_req = 0;       // Static request count
        t->dynm_req = 0;       // Dynamic request count
        t->post_req = 0;       // POST request count
        t->total_req = 0;      // Total request count

        time_stats dum;

        requestHandle(connfd, dum, t, global_log);

        free(t); // Cleanup
        Close(connfd); // Close the connection

    }
    return NULL;
}

// TODO: HW3 — Task 4: Add the UDP channel (see the UDP_* wrappers in segel.c).

// TODO: HW3 — Extend getargs() to parse the full argument list.

int main(int argc, char *argv[])
{
    // Create the global server log
    global_log = create_log();
    if (pthread_mutex_init(&lock, NULL) != 0) {
        perror("Failed to initialize mutex");
        return 1;
    }

    pthread_cond_init(&cond_not_full, NULL); //guaranteed to succeed and return 0.
    pthread_cond_init(&cond_not_empty, NULL); //guaranteed to succeed and return 0.

    pthread_t workers[NUM_WORKERS];

    initQueue(&connections_queue);

    for(long i = 0; i < NUM_WORKERS; i++){
        if (pthread_create(&workers[i], NULL, worker_function, (void*)(i + 1)) != 0) {
            perror("Failed to create thread");
            return 1;
        }
    }

    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;

    getargs(&port, argc, argv);

    listenfd = Open_listenfd(port);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t*) &clientlen);

        int arrival_time = 0;
        //gettimeofday(&arrival_time, NULL);

        pthread_mutex_lock(&lock);
        while(isFull(&connections_queue)){
            pthread_cond_wait(&cond_not_full, &lock);
        }

        enqueue(&connections_queue, (item){connfd, arrival_time});
        pthread_cond_signal(&cond_not_empty);
        pthread_mutex_unlock(&lock);



        // TODO: HW3 — Record the request arrival time here.

        // DEMO PURPOSE ONLY:
        // This is a dummy request handler that immediately processes the
        // request in the master thread without concurrency. Replace this with
        // logic that enqueues the connection so a worker thread handles it.


    }

    // Clean up the server log before exiting
    destroy_log(global_log);

    // TODO: HW3 — Add cleanup code for the thread pool and queue.

}

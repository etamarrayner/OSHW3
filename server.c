#include "segel.h"
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

//shared variables
CircularQueue connections_queue;
server_log global_log;
UDP_queue *p_queues;
int *busy, udp_fd;

pthread_mutex_t lock;
pthread_cond_t *p_conds;
pthread_cond_t cond_not_full;


void convert_int(char* str_in, int* num_out){
    char *bad;
    long temp = strtol(str_in, &bad, 10);
    if(*str_in == '\0' || *bad != '\0' || temp < 0){
        unix_error("Invalid command line arguments");
    }
    *num_out = (int) temp;
}

void convert_float(char* str_in, float* num_out){
    char *bad;
    long temp = strtof(str_in, &bad);
    if(*str_in == '\0' || *bad != '\0' || temp < 0){
        unix_error("Invalid command line arguments");
    }
    *num_out = temp;
}

// Parses command-line arguments
void getargs(int *TCPport, int *UDPport, int *num_workers,
        int *queue_size, float *log_sleep, int argc, char *argv[])
{
    if (argc < 6) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    convert_int(argv[1], TCPport);
    convert_int(argv[2], UDPport);
    convert_int(argv[3], num_workers);
    convert_int(argv[4], queue_size);
    convert_float(argv[5], log_sleep);
    //convert(argv[5], log_sleep);
    if(*TCPport < 1024 || *UDPport < 1024 || *TCPport == *UDPport){
        unix_error("Invalid ports");
    }
}

// TODO: HW3 — Task 1: Initialize the thread pool and request queue.
// This server currently handles all requests in the main thread.

void* worker_function(void* arg){

    threads_stats t = malloc(sizeof(struct Threads_stats));
    t->id = (int)arg;
    t->dynm_req=0;
    t->post_req=0;
    t->stat_req=0;
    t->total_req=0;

    while(1){
        pthread_mutex_lock(&lock);
        busy[t->id] = 0;
        while(isEmpty(&connections_queue) && isUDP_queueEmpty(&p_queues[t->id])){
            pthread_cond_wait(&p_conds[t->id], &lock);
        }

        busy[t->id] = 1;

        if(!isUDP_queueEmpty(&p_queues[t->id])){
            struct sockaddr_in temp = dequeueUDP(&p_queues[t->id]);
            pthread_mutex_unlock(&lock);
            UDPHandle(temp, t, udp_fd);
            continue;
        }

        struct timeval dispatch_time;
        gettimeofday(&dispatch_time, NULL);

        item curr_item = dequeue(&connections_queue);

        int connfd = curr_item.fd;
        curr_item.stats.task_dispatch = dispatch_time;

        pthread_cond_signal(&cond_not_full);

        pthread_mutex_unlock(&lock);

        requestHandle(connfd, curr_item.stats, t, global_log);
        Close(connfd); // Close the connection
    }
    free(t);
    return NULL;
}

// TODO: HW3 — Task 4: Add the UDP channel (see the UDP_* wrappers in segel.c).

// TODO: HW3 — Extend getargs() to parse the full argument list.

int main(int argc, char *argv[])
{
    int listenfd, connfd, TCPport, UDPport, clientlen, NUM_WORKERS, 
        queue_size;
    float f_log_sleep;
    getargs(&TCPport, &UDPport, &NUM_WORKERS, &queue_size, &f_log_sleep, argc, argv);

    struct timespec sleep_time;
    sleep_time.tv_sec = (time_t)(f_log_sleep);
    sleep_time.tv_nsec = (long)(((f_log_sleep) - sleep_time.tv_sec) * 1e9);

    busy = (int*)malloc(sizeof(int)*(1+NUM_WORKERS));
    p_queues = malloc(sizeof(UDP_queue)*(1+NUM_WORKERS));
    p_conds = malloc(sizeof(pthread_cond_t)*(1+NUM_WORKERS));
    for(int i = 1; i<= NUM_WORKERS; i++) {
        busy[i] = 1; 
        InitUDP_queue(&p_queues[i]);
        pthread_cond_init(&p_conds[i], NULL);
    }

    // Create the global server log
    global_log = create_log(sleep_time);
    if (pthread_mutex_init(&lock, NULL) != 0) {
        unix_error("Failed to initialize mutex");
    }

    pthread_cond_init(&cond_not_full, NULL); //guaranteed to succeed and return 0.

    struct sockaddr_in clientaddr, udpaddr;

    pthread_t workers[NUM_WORKERS];

    fd_set fd_in;

    initQueue(&connections_queue, queue_size);

    for(long i = 0; i < NUM_WORKERS; i++){
        if (pthread_create(&workers[i], NULL, worker_function, (void*)(i + 1)) != 0) {
            unix_error("Failed to create thread");
        }
    }

    listenfd = Open_listenfd(TCPport);
    udp_fd = UDP_Open(UDPport);

    int curr = 0;

    while (1) {
        time_stats request_stats;
        FD_ZERO(&fd_in); FD_SET(listenfd, &fd_in); FD_SET(udp_fd, &fd_in);
        int max = 1 + (udp_fd > listenfd ? udp_fd : listenfd);
        Select(max, &fd_in, NULL, NULL, NULL);

        if(FD_ISSET(udp_fd, &fd_in)){
            char input[32];
            int char_size;
            if((char_size = UDP_Read(udp_fd, &udpaddr, input, 1 + NUM_WORKERS/10)) < 0){
                printf("Read failed\n");
            }
            else{
                input[char_size] = '\0';
                int thread_id = atoi(input);
                if(thread_id > NUM_WORKERS || thread_id < 1){
                    printf("Invalid thread id: %d\n", thread_id);
                }
                else{
                    pthread_mutex_lock(&lock);
                    enqueueUDP(&p_queues[thread_id], udpaddr);
                    pthread_cond_signal(&p_conds[thread_id]);
                    pthread_mutex_unlock(&lock);
                }
            }
        }
        if(FD_ISSET(listenfd, &fd_in)){
            gettimeofday(&(request_stats.task_arrival), NULL);
            clientlen = sizeof(clientaddr);
            connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t*) &clientlen);
        
            pthread_mutex_lock(&lock);
            while(isFull(&connections_queue)){
                pthread_cond_wait(&cond_not_full, &lock);
            }

            enqueue(&connections_queue, (item){connfd, request_stats});
            for(int i = 0; i < NUM_WORKERS; i++){
                curr = (curr % NUM_WORKERS) + 1;
                if(!busy[curr]){
                    pthread_cond_signal(&p_conds[curr]);
                    break;
                }
            }
            pthread_mutex_unlock(&lock);
        }



        // TODO: HW3 — Record the request arrival time here.

        // DEMO PURPOSE ONLY:
        // This is a dummy request handler that immediately processes the
        // request in the master thread without concurrency. Replace this with
        // logic that enqueues the connection so a worker thread handles it.


    }

    // Clean up the server before exiting
    destroy_log(global_log);
    free(busy);
    free(p_conds);
    for(int i = 1; i <= NUM_WORKERS; i++) 
        deleteUDP_queue(&p_queues[i]);
    free(p_queues);

    // TODO: HW3 — Add cleanup code for the thread pool and queue.

}

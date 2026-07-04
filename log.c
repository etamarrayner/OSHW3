#include <stdlib.h>
#include <string.h>
#include "log.h"
#include <pthread.h>

// Opaque struct definition
typedef struct log_entry {
    char* data;
    struct log_entry* next;
}   log_entry_t;

struct server_log {
    // TODO: Implement internal log storage (e.g., dynamic buffer, linked list, etc.)
    log_entry_t* head;
    log_entry_t* tail;
    int curr_size;

    //reader-writer lock based on tutorial 8 slides 32-35
    int readers_inside, writers_inside, writers_waiting;
    pthread_cond_t read_allowed;
    pthread_cond_t write_allowed;
    pthread_mutex_t global_lock;


};

// Creates a new server log instance (stub)
server_log create_log() {
    // TODO: Allocate and initialize internal log structure
    server_log log = (server_log)malloc(sizeof(struct server_log));
    if(log == NULL){
        return NULL;
    }
    log->head = NULL;
    log->tail = NULL;
    log->curr_size = 0;

    log->readers_inside = 0;
    log->writers_inside = 0;
    log->writers_waiting = 0;

    pthread_cond_init(&log->read_allowed, NULL);
    pthread_cond_init(&log->write_allowed, NULL);
    pthread_mutex_init(&log->global_lock, NULL);

    return log;
}

// Destroys and frees the log (stub)
void destroy_log(server_log log) {
    log_entry_t* current = log->head;
    while (current != NULL) {
        log_entry_t* temp = current->next;
        free(current->data);
        free(current);
        current = temp;
    }

    pthread_mutex_destroy(&log->global_lock);
    pthread_cond_destroy(&log->read_allowed);
    pthread_cond_destroy(&log->write_allowed);

    free(log);
}

static void reader_lock(server_log log) {
    pthread_mutex_lock(&log->global_lock);
    while (log->writers_inside + log->writers_waiting> 0){
        pthread_cond_wait(&log->read_allowed, &log->global_lock);
    }
    log->readers_inside++;
    pthread_mutex_unlock(&log->global_lock);
}


static void reader_unlock(server_log log) {
    pthread_mutex_lock(&log->global_lock);
    log->readers_inside--;
    if (log->readers_inside == 0)
        pthread_cond_signal(&log->write_allowed);
    pthread_mutex_unlock(&log->global_lock);
}

static void writer_lock(server_log log) {
    pthread_mutex_lock(&log->global_lock);

    log->writers_waiting++;
    while (log->writers_inside + log->readers_inside > 0) {
        pthread_cond_wait(&log->write_allowed, &log->global_lock);
    }
    log->writers_waiting--;
    log->writers_inside++;
    pthread_mutex_unlock(&log->global_lock);
}

void writer_unlock(server_log log) {
    pthread_mutex_lock(&log->global_lock);
    log->writers_inside--;
    if (log->writers_inside == 0) {
        pthread_cond_broadcast(&log->read_allowed);
        pthread_cond_signal(&log->write_allowed);
    }
    pthread_mutex_unlock(&log->global_lock);
}


// Returns dummy log content as string (stub)
int get_log(server_log log, char** dst) {
    // TODO: Return the full contents of the log as a dynamically allocated string
    // This function should handle concurrent access

    reader_lock(log);

    int total_log_len = 0;
    log_entry_t* current = log->head;
    while(current != NULL){
        total_log_len += strlen(current->data);
        current = current->next;
    }
    *dst = (char*)malloc(total_log_len + 1); // Allocate for caller
    if (*dst == NULL) {
        reader_unlock(log);
        return 0;
    }
    (*dst)[0] = '\0';
    current = log->head;
    while(current != NULL){
        strcat(*dst, current->data);
        current = current->next;
    }
    reader_unlock(log);
    return total_log_len;
}

// Appends a new entry to the log (no-op stub)
void add_to_log(server_log log, const char* data, int data_len) {
    // TODO: Append the provided data to the log
    // This function should handle concurrent access
    log_entry_t* new_entry = (log_entry_t*)malloc(sizeof(log_entry_t));
    new_entry->data = (char*)malloc(data_len + 1);
    strcpy(new_entry->data, data);
    new_entry->next = NULL;
    writer_lock(log);
    if (log->head == NULL) {
        log->head = new_entry;
        log->tail = new_entry;
    } else {
        log->tail->next = new_entry;
        log->tail = new_entry;
    }
    log->curr_size++;
    writer_unlock(log);
}

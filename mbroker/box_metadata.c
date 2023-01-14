#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "box_metadata.h"

box_metadata_t *box_metadata_create(const char *name, const int max_sessions) {
    box_metadata_t *box = malloc(sizeof(box_metadata_t));
    ALWAYS_ASSERT(box != NULL, "Failed to alloc box_metadata");
    box->messages_written = 0;
    strcpy(box->name, name);
    pthread_mutex_init(&box->publishers_lock, NULL);
    pthread_mutex_init(&box->subscribers_lock, NULL);
    pthread_mutex_init(&box->publishers_count_lock, NULL);
    pthread_mutex_init(&box->subscribers_count_lock, NULL);
    pthread_mutex_init(&box->read_condvar_lock, NULL);
    pthread_cond_init(&box->read_condvar, NULL);
    box->publishers = calloc(max_sessions, sizeof(size_t));
    box->subscribers = calloc(max_sessions, sizeof(size_t));
    box->publishers_count = 0;
    box->subscribers_count = 0;
    return box;
}

void box_metadata_destroy(box_metadata_t *box) {
    pthread_mutex_destroy(&box->publishers_lock);
    pthread_mutex_destroy(&box->subscribers_lock);
    pthread_mutex_destroy(&box->publishers_count_lock);
    pthread_mutex_destroy(&box->subscribers_count_lock);
    pthread_mutex_destroy(&box->read_condvar_lock);
    pthread_cond_destroy(&box->read_condvar);
    free(box->name);
    free(box->subscribers);
    free(box->publishers);
    free(box);
}
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

int box_holder_create(box_holder_t *holder, const size_t max_boxes) {
    holder->current_size = 0;
    holder->max_size = max_boxes;
    holder->boxes = calloc(max_boxes, sizeof(box_metadata_t));
    if (holder->boxes == NULL) {
        return -1; // failed to alloc boxes
    }
    return 0;
}

void box_holder_insert(box_holder_t *holder, const box_metadata_t *box) {
    ALWAYS_ASSERT(holder->current_size < holder->max_size,
                  "Already inserted max size of boxes");
    holder->boxes[holder->current_size] = box;
    holder->current_size++;
}

void box_holder_remove(box_holder_t *holder, const char *name) {
    for (size_t i = 0; i < holder->current_size; i++) {
        box_metadata_t *box = holder->boxes[i];
        if (strcmp(box->name, name) == 0) {
            holder->boxes[i] = NULL;
            box_metadata_destroy(box);
            for (size_t j = i; j < holder->current_size - 1; j++) {
                holder->boxes[j] = holder->boxes[j + 1];
            }
            holder->current_size--;
            return;
        }
    }
    WARN("No boxes with the name: %s were removed", name);
}

box_metadata_t *box_holder_find_box(box_holder_t *holder, const char *name) {
    for (size_t i = 0; i < holder->current_size; i++) {
        box_metadata_t *box = holder->boxes[i];
        if (strcmp(box->name, name) == 0) {
            return box; // found box
        }
    }
    return NULL; // no box was found
}
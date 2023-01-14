#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "../utils/betterassert.h"
#include "box_metadata.h"

box_metadata_t *box_metadata_create(const char *name,
                                    const size_t max_sessions) {
    box_metadata_t *box = malloc(sizeof(box_metadata_t));
    ALWAYS_ASSERT(box != NULL, "Failed to alloc box_metadata");
    pthread_mutex_init(&box->has_publisher_lock, NULL);
    pthread_mutex_init(&box->subscribers_lock, NULL);
    pthread_mutex_init(&box->publisher_idx_lock, NULL);
    pthread_mutex_init(&box->subscribers_count_lock, NULL);
    // pthread_mutex_init(&box->read_condvar_lock, NULL);
    pthread_mutex_init(&box->total_message_size_lock, NULL);
    pthread_cond_init(&box->read_condvar, NULL);
    strcpy(box->name, name);
    box->publisher_idx = 0;
    box->subscribers = calloc(max_sessions, sizeof(size_t));
    box->has_publisher = false;
    box->subscribers_count = 0;
    box->total_message_size = 0;
    return box;
}

void box_metadata_destroy(box_metadata_t *box) {
    pthread_mutex_destroy(&box->has_publisher_lock);
    pthread_mutex_destroy(&box->subscribers_lock);
    pthread_mutex_destroy(&box->publisher_idx_lock);
    pthread_mutex_destroy(&box->subscribers_count_lock);
    // pthread_mutex_destroy(&box->read_condvar_lock);
    pthread_mutex_destroy(&box->total_message_size_lock);
    pthread_cond_destroy(&box->read_condvar);
    free(box->name);
    free(box->subscribers);
    free(box);
}

int box_holder_create(box_holder_t *holder, const size_t max_boxes) {
    DEBUG("Creating box holder");
    holder->current_size = 0;
    holder->max_size = max_boxes;
    holder->boxes = calloc(max_boxes, sizeof(box_metadata_t));
    pthread_mutex_init(&holder->lock, NULL);
    if (holder->boxes == NULL) {
        return -1; // failed to alloc boxes
    }
    return 0;
}

void box_holder_insert(box_holder_t *holder, box_metadata_t *box) {
    pthread_mutex_lock(&holder->lock);
    DEBUG("Inserting box %s", box->name);
    ALWAYS_ASSERT(holder->current_size < holder->max_size,
                  "Already inserted max size of boxes");
    holder->boxes[holder->current_size] = box;
    holder->current_size++;
    pthread_mutex_unlock(&holder->lock);
}

int box_holder_remove(box_holder_t *holder, const char *name) {
    pthread_mutex_lock(&holder->lock);
    DEBUG("Removing box %s", name);
    for (size_t i = 0; i < holder->current_size; i++) {
        box_metadata_t *box = holder->boxes[i];
        if (strcmp(box->name, name) == 0) {
            holder->boxes[i] = NULL;
            box_metadata_destroy(box);
            for (size_t j = i; j < holder->current_size - 1; j++) {
                holder->boxes[j] = holder->boxes[j + 1];
            }
            holder->current_size--;
            pthread_mutex_unlock(&holder->lock);
            return 0;
        }
    }
    WARN("No boxes with the name: %s were removed", name);
    pthread_mutex_unlock(&holder->lock);
    return -1;
}

box_metadata_t *box_holder_find_box(box_holder_t *holder, const char *name) {
    pthread_mutex_lock(&holder->lock);
    for (size_t i = 0; i < holder->current_size; i++) {
        box_metadata_t *box = holder->boxes[i];
        if (strcmp(box->name, name) == 0) {
            return box; // found box
        }
    }
    return NULL; // no box was found
    pthread_mutex_unlock(&holder->lock);
}
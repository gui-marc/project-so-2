#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "../utils/betterassert.h"
#include "box_metadata.h"

box_metadata_t *box_metadata_create(const char *name,
                                    const size_t max_sessions) {
    DEBUG("Creating box metadata for %s", name);
    box_metadata_t *box = calloc(1, sizeof(box_metadata_t));
    ALWAYS_ASSERT(box != NULL, "Failed to alloc box_metadata");
    ALWAYS_ASSERT(
        pthread_mutex_init(&box->has_publisher_lock, NULL) == 0 &&
            pthread_mutex_init(&box->subscribers_lock, NULL) == 0 &&
            pthread_mutex_init(&box->publisher_idx_lock, NULL) == 0 &&
            pthread_mutex_init(&box->subscribers_count_lock, NULL) == 0 &&
            // pthread_mutex_init(&box->read_condvar_lock, NULL) == 0 &&
            pthread_mutex_init(&box->total_message_size_lock, NULL) == 0 &&
            pthread_cond_init(&box->read_condvar, NULL) == 0,
        "Failed to init mutexes");

    strcpy(box->name, name);
    box->publisher_idx = 0;
    box->subscribers = calloc(max_sessions, sizeof(size_t));
    box->has_publisher = false;
    box->subscribers_count = 0;
    box->total_message_size = 0;
    return box;
}

void box_metadata_destroy(box_metadata_t *box) {
    DEBUG("Destroying mutexes");
    pthread_mutex_destroy(&box->has_publisher_lock);
    pthread_mutex_destroy(&box->subscribers_lock);
    pthread_mutex_destroy(&box->publisher_idx_lock);
    pthread_mutex_destroy(&box->subscribers_count_lock);
    // pthread_mutex_destroy(&box->read_condvar_lock);
    pthread_mutex_destroy(&box->total_message_size_lock);
    pthread_cond_destroy(&box->read_condvar);
    DEBUG("Finished destroying mutexes");
    free(box);
}

int box_holder_create(box_holder_t *holder, const size_t max_boxes) {
    DEBUG("Holder: %p", holder);
    holder->current_size = 0;
    holder->max_size = max_boxes;
    DEBUG("Creating box holder {%lu,%lu}", holder->current_size,
          holder->max_size);
    holder->boxes = calloc(max_boxes, sizeof(box_metadata_t));
    ALWAYS_ASSERT(pthread_mutex_init(&holder->lock, NULL) == 0,
                  "Failed to init holder mutex");
    if (holder->boxes == NULL) {
        return -1; // failed to alloc boxes
    }
    return 0;
}

void box_holder_insert(box_holder_t *holder, box_metadata_t *box) {
    DEBUG("Inserting box %s", box->name);
    DEBUG("Holder: %p", holder);
    pthread_mutex_lock(&holder->lock);
    DEBUG("Passed holder lock");
    ALWAYS_ASSERT(holder->current_size < holder->max_size,
                  "Already inserted max size of boxes (%lu of %lu)",
                  holder->current_size, holder->max_size);
    holder->boxes[holder->current_size] = box;
    holder->current_size++;
    pthread_mutex_unlock(&holder->lock);
    DEBUG("Finished inserting box");
}

int box_holder_remove(box_holder_t *holder, const char *name) {
    pthread_mutex_lock(&holder->lock);
    DEBUG("Removing box %s", name);
    for (size_t i = 0; i < holder->current_size; i++) {
        box_metadata_t *box = holder->boxes[i];
        DEBUG("For loop in box '%s'", box->name);
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
            pthread_mutex_unlock(&holder->lock);
            return box; // found box
        }
    }
    pthread_mutex_unlock(&holder->lock);
    return NULL; // no box was found
}
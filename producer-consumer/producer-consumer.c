#include "producer-consumer.h"
#include "logging.h"
#include <stdio.h>

int pcq_create(pc_queue_t *queue, size_t capacity) {
    queue->pcq_buffer = calloc(1, capacity);
    if (queue->pcq_buffer == NULL) {
        return -1;
    }

    queue->pcq_capacity = capacity;
    return 0;
};

int pcq_destroy(pc_queue_t *queue) {
    (void)queue;
    WARN("unimplemented"); // TODO: implement
    return -1;
};

int pcq_enqueue(pc_queue_t *queue, void *elem) {
    (void)queue;
    (void)elem;
    WARN("unimplemented"); // TODO: implement
    return -1;
};

void *pcq_dequeue(pc_queue_t *queue) {
    (void)queue;
    WARN("unimplemented"); // TODO: implement
    return queue;
};
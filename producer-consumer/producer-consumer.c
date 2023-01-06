#include "producer-consumer.h"
#include "logging.h"
#include <stdio.h>

int pcq_create(pc_queue_t *queue, size_t capacity) {
    (void)queue;
    (void)capacity;
    WARN("unimplemented"); // TODO: implement
    return -1;
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
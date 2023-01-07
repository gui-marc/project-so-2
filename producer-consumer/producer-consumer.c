#include <semaphore.h>
#include <stdio.h>

#include "logging.h"
#include "producer-consumer.h"

static sem_t sem_has_space;
static sem_t sem_has_element;

int pcq_create(pc_queue_t *queue, size_t capacity) {
    sem_init(&sem_has_space, 0,
             (unsigned int)capacity); // Starts with all empty
    sem_init(&sem_has_element, 0, 0); // Starts with 0 elements

    // Initializes the vector with the capacity
    queue->pcq_buffer = calloc(1, capacity);
    if (queue->pcq_buffer == NULL) {
        WARN("no memory to allocate pcq_queue buffer");
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
    // Waits until there is free space in the queue
    sem_wait(&sem_has_space);

    // Inserts a new element at the tail
    if (queue->pcq_tail >= queue->pcq_capacity - 1) {
        queue->pcq_buffer[0] = elem;
        queue->pcq_tail = 1;
    } else {
        queue->pcq_buffer[queue->pcq_tail + 1] = elem;
        queue->pcq_tail++;
    }
    queue->pcq_current_size++;

    // Tell other threads that one element was added to the queue
    sem_post(&sem_has_element);

    // Ok
    return 0;
};

void *pcq_dequeue(pc_queue_t *queue) {
    // Waits until there is at least 1 element in the queue
    sem_wait(&sem_has_element);

    // Pops the element at the head
    void *last = queue->pcq_buffer[queue->pcq_head];
    queue->pcq_buffer[queue->pcq_head] = NULL;
    // If is the last element and still has capacity (implicit in the semaphore)
    // the next element (head) will be at the start of the vector
    if (queue->pcq_head == queue->pcq_capacity - 1) {
        queue->pcq_head = 0;
    } else {
        queue->pcq_head++;
    }
    queue->pcq_current_size--;

    // Tell the other threads that there is one more free space in the queue
    sem_post(&sem_has_space);
    return last;
};
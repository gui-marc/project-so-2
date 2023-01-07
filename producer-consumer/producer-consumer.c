#include <pthread.h>
#include <semaphore.h>

#include "logging.h"
#include "producer-consumer.h"

// Todo: add more error treatment

int pcq_create(pc_queue_t *queue, size_t capacity) {
    // There is no need to lock mutexes because at this point no thread was
    // created

    // Initializes mutexes and condvar
    if (pthread_mutex_init(&queue->pcq_current_size_lock, NULL) != 0 ||
        pthread_mutex_init(&queue->pcq_head_lock, NULL) != 0 ||
        pthread_mutex_init(&queue->pcq_tail_lock, NULL) != 0 ||
        pthread_mutex_init(&queue->pcq_popper_condvar_lock, NULL) != 0 ||
        pthread_mutex_init(&queue->pcq_pusher_condvar_lock, NULL) != 0 ||
        pthread_cond_init(&queue->pcq_popper_condvar, NULL) != 0 ||
        pthread_cond_init(&queue->pcq_pusher_condvar, NULL) != 0) {
        WARN("error while initializing mutexes and condvar\n");
        return -1;
    }

    // Init variables as 0
    queue->pcq_current_size = 0;
    queue->pcq_head = 0;
    queue->pcq_tail = 0;

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
    // There is no need to lock or unlock mutexes because at this point all
    // threads were closed

    // Frees all buffer related memory
    for (size_t i = 0; i < queue->pcq_capacity; i++) {
        free(queue->pcq_buffer[i]);
    }

    // Destroys all mutexes and condvar
    if (pthread_mutex_destroy(&queue->pcq_current_size_lock) != 0 ||
        pthread_mutex_destroy(&queue->pcq_head_lock) != 0 ||
        pthread_mutex_destroy(&queue->pcq_tail_lock) != 0 ||
        pthread_mutex_destroy(&queue->pcq_popper_condvar_lock) != 0 ||
        pthread_mutex_destroy(&queue->pcq_pusher_condvar_lock) != 0 ||
        pthread_cond_destroy(&queue->pcq_popper_condvar) != 0 ||
        pthread_cond_destroy(&queue->pcq_pusher_condvar) != 0) {
        WARN("error while destroying mutexes and condvar\n");
        return -1;
    }

    return 0;
};

int pcq_enqueue(pc_queue_t *queue, void *elem) {
    pthread_mutex_lock(&queue->pcq_current_size_lock);
    // If the queue is full
    while (queue->pcq_current_size == queue->pcq_capacity) {
        // Waits until there is free space in the queue
        pthread_cond_wait(&queue->pcq_pusher_condvar,
                          &queue->pcq_current_size_lock);
    }

    // Inserts a new element at the tail
    pthread_mutex_lock(&queue->pcq_tail_lock);
    if (queue->pcq_tail >= queue->pcq_capacity - 1) {
        queue->pcq_buffer[0] = elem;
        queue->pcq_tail = 1;
    } else {
        queue->pcq_buffer[queue->pcq_tail + 1] = elem;
        queue->pcq_tail++;
    }
    pthread_mutex_unlock(&queue->pcq_tail_lock);

    // Increment the current size
    queue->pcq_current_size++;
    pthread_mutex_unlock(&queue->pcq_current_size_lock);

    // Emit the signal that other threads can pop a element
    pthread_cond_signal(&queue->pcq_popper_condvar);

    // Ok
    return 0;
};

void *pcq_dequeue(pc_queue_t *queue) {
    pthread_mutex_lock(&queue->pcq_current_size_lock);
    // If the queue is empty
    while (queue->pcq_current_size == 0) {
        // Waits until there is at least 1 element in the queue
        pthread_cond_wait(&queue->pcq_popper_condvar,
                          &queue->pcq_current_size_lock);
    }

    // Pops the element at the head
    pthread_mutex_lock(&queue->pcq_head_lock);
    void *last = queue->pcq_buffer[queue->pcq_head];
    queue->pcq_buffer[queue->pcq_head] = NULL;
    // If is the last element and still has capacity (implicit in the semaphore)
    // the next element (head) will be at the start of the vector
    if (queue->pcq_head == queue->pcq_capacity - 1) {
        queue->pcq_head = 0;
    } else {
        queue->pcq_head++;
    }
    pthread_mutex_unlock(&queue->pcq_head_lock);

    // Decrease the current size
    queue->pcq_current_size--;
    pthread_mutex_unlock(&queue->pcq_current_size_lock);

    // Tell the other threads that there is one more free space in the queue
    pthread_cond_signal(&queue->pcq_pusher_condvar);
    return last;
};
#ifndef __BOX_METADATA_T_H__
#define __BOX_METADATA_T_H__

#include "../protocol/protocols.h"
#include <pthread.h>
#include <stdint.h>

typedef struct box_metadata_t {
    char name[BOX_NAME_SIZE];

    int messages_written;

    pthread_mutex_t read_condvar_lock;
    pthread_cond_t read_condvar;

    int publishers_count;
    pthread_mutex_t publishers_count_lock;

    int *publishers;
    pthread_mutex_t publishers_lock;

    int subscribers_count;
    pthread_mutex_t subscribers_count_lock;

    int *subscribers;
    pthread_mutex_t subscribers_lock;
} box_metadata_t;

/**
 * @brief Creates a box metadata structure
 *
 * @param name the box name
 * @param max_sessions the max sessions of the mbroker
 * @return box_metadata_t* the box metadata created
 */
box_metadata_t *box_metadata_create(const char *name, const int max_sessions);

/**
 * @brief Destroys a box metadata
 *
 * @param box
 */
void box_metadata_destroy(box_metadata_t *box);

#endif // __BOX_METADATA_T_H__
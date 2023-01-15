#ifndef __BOX_METADATA_T_H__
#define __BOX_METADATA_T_H__

#include "../protocol/protocols.h"
#include "utils.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
/**
 * @brief Represents a box in the mbroker. Contains all info related to the box.
 */
typedef struct box_metadata_t {
    char name[BOX_NAME_SIZE];

    size_t total_message_size;
    pthread_mutex_t total_message_size_lock;

    // pthread_mutex_t read_condvar_lock;
    pthread_cond_t read_condvar;

    pthread_t publisher_idx;
    pthread_mutex_t publisher_idx_lock;

    bool has_publisher;
    pthread_mutex_t has_publisher_lock;

    size_t subscribers_count;
    pthread_mutex_t subscribers_count_lock;

    size_t *subscribers;
    pthread_mutex_t subscribers_lock;
} box_metadata_t;

/**
 * @brief Creates a box metadata structure
 *
 * @param name the box name
 * @param max_sessions the max sessions of the mbroker
 * @return box_metadata_t* the box metadata created
 */
box_metadata_t *box_metadata_create(const char *name,
                                    const size_t max_sessions);

/**
 * @brief Destroys a box metadata
 *
 * @param box
 */
void box_metadata_destroy(box_metadata_t *box);

/**
 * @brief Holds the mbroker created boxes
 */
typedef struct box_holder_t {
    box_metadata_t **boxes;
    size_t current_size;
    size_t max_size;

    pthread_mutex_t lock;
} box_holder_t;

/**
 * @brief Creates a box_holder
 *
 * @param the already allocated holder
 * @param max_boxes
 * @return int 0 if was successful and -1 otherwise
 */
int box_holder_create(box_holder_t *holder, const size_t max_boxes);

/**
 * @brief Inserts a box metadata in the box holder
 *
 * @param holder to insert at
 * @param box to be inserted
 */
void box_holder_insert(box_holder_t *holder, box_metadata_t *box);

/**
 * @brief Removes a box from the holder
 *
 * @param name of the box to remove
 * @return 0 if it was sucessfully removed and -1 otherwise
 */
int box_holder_remove(box_holder_t *holder, const char *name);

/**
 * @brief Finds a box with the given name
 *
 * @param name the box name
 * @return box_metadata_t* if the was box found or NULL otherwise
 */
box_metadata_t *box_holder_find_box(box_holder_t *holder, const char *name);

#endif // __BOX_METADATA_T_H__
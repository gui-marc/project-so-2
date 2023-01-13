#ifndef __REQUESTS_H__
#define __REQUESTS_H__

#include "producer-consumer.h"

/**
 * Will listen to the queue and solve a request if it is made
 *
 * @param queue the current producer-consumer queue
 */
void *listen_for_requests(void *queue);

/**
 * Receives a request code and redirects to the method that will handle
 * that request
 *
 * @param request_code the code of the request @link(protocols.h)
 * @param protocol the string containing the other parameters in the request
 */
void parse_request(queue_obj_t *obj);

/**
 * Register a publisher
 *
 * @param protocol the string containing the other parameters in the request
 */
void register_publisher(queue_obj_t *obj);

/**
 * Register a subscriber
 *
 * @param protocol the string containing the other parameters in the request
 */
void register_subscriber(queue_obj_t *obj);

/**
 * Creates a message box in the TFS
 *
 * @param protocol the string containing the other parameters in the request
 */
void create_box(queue_obj_t *obj);

/**
 * Removes a message box in the TFS
 *
 * @param protocol the string containing the other parameters in the request
 */
void remove_box(queue_obj_t *obj);

/**
 * List all message boxes in the TFS
 *
 * @param protocol the string containing the other parameters in the request
 */
void list_boxes(queue_obj_t *obj);

#endif
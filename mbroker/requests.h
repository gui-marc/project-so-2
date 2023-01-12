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
void parse_request(uint8_t request_code, protocol_t *protocol);

/**
 * Register a publisher
 *
 * @param protocol the string containing the other parameters in the request
 */
void register_publisher(protocol_t *protocol);

/**
 * Register a subscriber
 *
 * @param protocol the string containing the other parameters in the request
 */
void register_subscriber(protocol_t *protocol);

/**
 * Creates a message box in the TFS
 *
 * @param protocol the string containing the other parameters in the request
 */
void create_box(protocol_t *protocol);

/**
 * Removes a message box in the TFS
 *
 * @param protocol the string containing the other parameters in the request
 */
void remove_box(protocol_t *protocol);

/**
 * List all message boxes in the TFS
 *
 * @param protocol the string containing the other parameters in the request
 */
void list_boxes(protocol_t *protocol);

#endif
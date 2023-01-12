#include <stdint.h>
#include <stdlib.h>

#ifndef __PROTOCOLS__
#define __PROTOCOLS__

#define MKFIFO_PERMS 0640

#define NAMED_PIPE_PATH_SIZE 256
#define MESSAGE_SIZE 1024
#define BOX_NAME_SIZE 32

#define CODE_REGISTER_PUBLISHER 1
#define CODE_REGISTER_SUBSCRIBER 2
#define CODE_CREATE_BOX_REQUEST 3
#define CODE_CREATE_BOX_RESPONSE 4
#define CODE_REMOVE_BOX_REQUEST 5
#define CODE_REMOVE_BOX_RESPONSE 6
#define CODE_LIST_BOXES_REQUEST 7
#define CODE_LIST_BOXES_RESPONSE 8
#define CODE_PUBLISHER_MESSAGE 9
#define CODE_SUBSCRIBER_MESSAGE 10

struct named_pipes {
    int write_fd;
    int read_fd;
};

/*
 * Client function: creates a write-only pipe to talk to the server
 * and a read-only pipe to listen to the server.
 * TODO: Assinaturas de funções
 */
void client_create_pipes(
    const char client_named_pipe_path[NAMED_PIPE_PATH_SIZE]);

// TODO: asinatura desta funcao
struct named_pipes
client_open_pipes(const char client_named_pipe_path[NAMED_PIPE_PATH_SIZE]);

// TODO: ver comentairo acima
struct named_pipes
server_open_pipes(const char client_named_pipe_path[NAMED_PIPE_PATH_SIZE]);

/**
 * Protocol
 *
 * Defines a base structure for all protocols.
 * All protocols must have the `code` attribute
 */
typedef struct protocol_base_t {
    uint8_t code;
} protocol_base_t;

typedef struct protocol_t {
    struct protocol_base_t base;
} protocol_t;

typedef struct request_protocol_t {
    struct protocol_base_t base;
    char client_named_pipe_path[NAMED_PIPE_PATH_SIZE];
    char box_name[BOX_NAME_SIZE];
} request_protocol_t;

typedef struct response_protocol_t {
    struct protocol_base_t base;
    int32_t return_code;
    char error_message[MESSAGE_SIZE];
} response_protocol_t;

typedef struct list_boxes_request_protocol_t {
    struct protocol_base_t base;
    char client_named_pipe_path[NAMED_PIPE_PATH_SIZE];
} list_boxes_request_protocol_t;

typedef struct list_boxes_response_protocol_t {
    struct protocol_base_t base;
    uint8_t last;
    char box_name[32];
    uint64_t box_size;
    uint64_t n_publishers;
    uint64_t n_subscribers;
} list_boxes_response_protocol_t;

typedef struct message_protocol_t {
    struct protocol_base_t base;
    char message[MESSAGE_SIZE];
} message_protocol_t;

/**
 * Creates a protocol string to register a publisher
 *
 * @param client_named_pipe_path string (char[NAMED_PIPE_PATH_SIZE]) containing
 * the path to the fifo
 * @param box_name the name of the box that the publisher will publish the
 * message in
 */
void *register_publisher_protocol(const char *client_named_pipe_path,
                                  const char *box_name);

/**
 * Creates a protocol string to register a subscriber
 *
 * @param client_named_pipe_path string (char[NAMED_PIPE_PATH_SIZE]) containing
 * the path to the fifo
 * @param box_name the name of the box that the subscriber will listen for
 * messages
 */
void *register_subscriber_protocol(const char *client_named_pipe_path,
                                   const char *box_name);

/**
 * Creates a protocol string to request creation of a box
 *
 * @param client_named_pipe_path string (char[NAMED_PIPE_PATH_SIZE]) containing
 * the path to the fifo
 * @param box_name the name of the box that will be created
 */
void *create_box_request_protocol(const char *client_named_pipe_path,
                                  const char *box_name);

/**
 * Creates a protocol string to respond to a box creation request
 *
 * @param return_code 0 if it was successfully created and -1 otherwise
 * @param error_message if no error is '\0' otherwise, must send an error
 * message
 */
void *create_box_response_protocol(int32_t return_code,
                                   const char *error_message);

/**
 * Creates a protocol string to request a box removal
 *
 * @param client_named_pipe_path string (char[NAMED_PIPE_PATH_SIZE]) containing
 * the path to the fifo
 * @param box_name the box to be removed
 */
void *remove_box_request_protocol(const char *client_named_pipe_path,
                                  const char *box_name);

/**
 * Creates a protocol string to respond to a box removal request
 *
 * @param return_code 0 if it was successfully removed and -1 otherwise
 * @param error_message if no error is '\0' otherwise, must send an error
 * message
 */
void *remove_box_response_protocol(const int32_t return_code,
                                   const char *error_message);

/**
 * Creates a protocol string to request a list of boxes
 *
 * @param client_named_pipe_path string (char[NAMED_PIPE_PATH_SIZE]) containing
 * the path to the fifo
 */
void *list_boxes_request_protocol(const char *client_named_pipe_path);

/**
 * Creates a protocol string to respond to a list_boxes_request
 *
 * @param last 1 if is the last box from the list or if there are no boxes and 0
 * otherwise
 * @param box_name name of the box (\0 if there are no boxes)
 * @param box_size size of the box
 * @param n_publishers publishers connected to the box
 * @param n_subscribers subscribers connected to the box
 */
void *list_boxes_response_protocol(const uint8_t last, const char *box_name,
                                   const uint64_t box_size,
                                   const uint64_t n_publishers,
                                   const uint64_t n_subscribers);

/**
 * Creates a protocol string that the publisher uses
 *
 * @param message the message to be sent to the server
 */
void *publisher_message_protocol(const char *message);

/**
 * Creates a protocol string that the subscriber uses
 *
 * @param message the message to be sent to the server
 */
void *subscriber_message_protocol(const char *message);

#endif
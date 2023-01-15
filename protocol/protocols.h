#include <stdint.h>
#include <stdlib.h>

#ifndef __PROTOCOLS__
#define __PROTOCOLS__

#define STR_MATCH(str1, str2) (strcmp(str1, str2) == 0)

#define MKFIFO_PERMS 0640

#define NPIPE_PATH_SIZE 256
#define BOX_NAME_SIZE 32

// Exported from fs/config.h, since we don't (and shouldn't) have access to this
// header. Should be kept in sync with its equivalent there.
#define MAX_FILE_NAME (40)
#define MSG_SIZE 1024

/**
 * Protocol Codes
 */
typedef enum codes_e {
    REGISTER_PUBLISHER = 1,
    REGISTER_SUBSCRIBER,
    CREATE_BOX_REQUEST,
    CREATE_BOX_RESPONSE,
    REMOVE_BOX_REQUEST,
    REMOVE_BOX_RESPONSE,
    LIST_BOXES_REQUEST,
    LIST_BOXES_RESPONSE,
    PUBLISHER_MESSAGE,
    SUBSCRIBER_MESSAGE
} CODES;

/**
 * Error Messages
 */
#define ERR_BOX_NOT_FOUND "Box not found."
#define ERR_BOX_ALREADY_EXISTS "Box already exists."
#define ERR_BOX_CREATION "An error ocurred while creating the box."

/**
 * Protocol
 *
 * Defines a base structure for all protocols.
 * All protocols must have the `code` attribute
 */

// Used for sub/pub register request
typedef struct __attribute__((__packed__)) request_proto_t {
    char client_named_pipe_path[NPIPE_PATH_SIZE];
    char box_name[BOX_NAME_SIZE];
} request_proto_t;

// All these protocol messages are the same
#define register_pub_proto_t request_proto_t
#define register_sub_proto_t request_proto_t
#define create_box_proto_t request_proto_t
#define remove_box_proto_t request_proto_t

typedef struct __attribute__((__packed__)) response_proto_t {
    int32_t return_code;
    char error_msg[MSG_SIZE];
} response_proto_t;

#define create_box_response_proto_t response_proto_t
#define remove_box_response_proto_t response_proto_t

/**
 * Protocol packed struct (without paddings) to send a list boxes request
 */
typedef struct __attribute__((__packed__)) list_boxes_request_proto_t {
    char client_named_pipe_path[NPIPE_PATH_SIZE];
} list_boxes_request_proto_t;

/**
 * Protocol packed struct (without paddings) to send a list boxes response
 */
typedef struct __attribute__((__packed__)) list_boxes_response_proto_t {
    uint8_t last;
    char box_name[BOX_NAME_SIZE];
    uint64_t box_size;
    uint64_t n_publishers;
    uint64_t n_subscribers;
} list_boxes_response_proto_t;

/**
 * Protocol packed struct (without paddings) to send a basic message (string)
 */
typedef struct __attribute__((__packed__)) basic_msg_proto_t {
    char msg[MSG_SIZE];
} basic_msg_proto_t;

#define publisher_msg_proto_t basic_msg_proto_t
#define subscriber_msg_proto_t basic_msg_proto_t

uint8_t recv_opcode(const int fd);

/**
 * @brief Send a protocol packed struct as an array of bytes
 *
 * @details It follows the following structure:
 *
 * [ uint8_t opcode | ...protocol ]
 *
 * The opcode is the code of the protocol and the protocol is an structure with
 * variable size. You can get the protocol size with the @link proto_size method
 *
 * In case of an error, function will panic and quit, unless as described in
 * return.
 *
 * @return -1 if failed to write due to EPIPE, 0 otherwise
 */
int send_proto_string(const int fd, const uint8_t opcode, const void *proto);

/**
 * @brief Creates a named pipe
 *
 * @param npipe_path Path of the named pipe
 * @return * void
 */
void create_pipe(const char npipe_path[NPIPE_PATH_SIZE]);

/**
 * @brief Receives an opcode an reads the rest of the buffer with the right
 * protocol size
 *
 * @param rx the read file descriptor of the pipe
 * @param opcode the protocol opcode
 * @return void* the parsed protocol
 */
void *parse_protocol(const int rx, const uint8_t opcode);

/**
 * @brief Opens a named pipe with the passed flags
 *
 * @param npipe_path the path of the named pipe
 * @param _flags flags to open the pipe
 * @return int the file descriptor
 */
int open_pipe(const char npipe_path[NPIPE_PATH_SIZE], int _flags);

/**
 * @brief Creates a protocol for a request
 *
 * @param client_named_pipe_path the path to the client named pipe
 * @param box_name the name of the associated box
 * @return void* the request protocol reference
 */
void *request_proto(const char *client_named_pipe_path, const char *box_name);

/**
 * @brief Creates a protocol for a response
 *
 * @param return_code 0 if it was successful and -1 otherwise
 * @param error_message a message if it had an error
 * @return void* the response protocol reference
 */
void *response_proto(int32_t return_code, const char *error_message);

/**
 * Creates a protocol string to request a list of boxes
 *
 * @param client_named_pipe_path string (char[NPIPE_PATH_SIZE]) containing
 * the path to the fifo
 */
void *list_boxes_request_proto(const char *client_named_pipe_path);

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
void *list_boxes_response_proto(const uint8_t last, const char *box_name,
                                const uint64_t box_size,
                                const uint64_t n_publishers,
                                const uint64_t n_subscribers);

/**
 * Creates a protocol string that the publisher and the subscriber uses
 *
 * @param message the message to be sent to the server
 */
void *message_proto(const char *message);

/**
 * Returns the right size of a protocol
 *
 * @param code protocol code
 */
size_t proto_size(CODES code);

#endif
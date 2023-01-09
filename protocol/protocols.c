#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logging.h"
#include "protocols.h"

char *alloc_protocol_message() {
    char *protocol_message = calloc(PROTOCOL_MESSAGE_LENGTH, sizeof(char));
    if (protocol_message == NULL) {
        WARN("failed to allocate space for protocol message");
        return NULL;
    }
    return protocol_message;
}

char *register_publisher_protocol(const char client_named_pipe_path[256],
                                  const char box_name[32]) {
    char *protocol_message = alloc_protocol_message();
    sprintf(protocol_message, "%d|%s|%s", CODE_REGISTER_PUBLISHER,
            client_named_pipe_path, box_name);
    return protocol_message;
}

char *register_subscriber_protocol(const char client_named_pipe_path[NAMED_PIPE_PATH_SIZE],
                                   const char box_name[32]) {
    char *protocol_message = alloc_protocol_message();
    sprintf(protocol_message, "%d|%s|%s", CODE_REGISTER_SUBSCRIBER,
            client_named_pipe_path, box_name);
    return protocol_message;
}

char *create_box_request_protocol(const char client_named_pipe_path[NAMED_PIPE_PATH_SIZE],
                                  const char box_name[32]) {
    char *protocol_message = alloc_protocol_message();
    sprintf(protocol_message, "%d|%s|%s", CODE_CREATE_BOX_REQUEST,
            client_named_pipe_path, box_name);
    return protocol_message;
}

char *create_box_response_protocol(int32_t return_code,
                                   const char error_message[1024]) {
    char *protocol_message = alloc_protocol_message();
    sprintf(protocol_message, "%d|%d|%s", CODE_REGISTER_PUBLISHER, return_code,
            error_message);
    return protocol_message;
}

char *remove_box_request_protocol(const char client_named_pipe_path[NAMED_PIPE_PATH_SIZE],
                                  const char box_name[32]) {
    char *protocol_message = alloc_protocol_message();
    sprintf(protocol_message, "%d|%s|%s", CODE_REMOVE_BOX_REQUEST,
            client_named_pipe_path, box_name);
    return protocol_message;
}

char *remove_box_response_protocol(const int32_t return_code,
                                   const char error_message[1024]) {
    char *protocol_message = alloc_protocol_message();
    sprintf(protocol_message, "%d|%d|%s", CODE_REMOVE_BOX_RESPONSE, return_code,
            error_message);
    return protocol_message;
}

char *list_boxes_request_protocol(const char client_named_pipe_path[NAMED_PIPE_PATH_SIZE]) {
    char *protocol_message = alloc_protocol_message();
    sprintf(protocol_message, "%d|%s", CODE_LIST_BOXES_REQUEST,
            client_named_pipe_path);
    return protocol_message;
}

char *list_boxes_response_protocol(const uint8_t last, const char box_name[32],
                                   const uint64_t box_size, const uint64_t n_publishers,
                                   const uint64_t n_subscribers) {
    char *protocol_message = alloc_protocol_message();
    sprintf(protocol_message, "%d|%d|%s|%lu|%lu|%lu", CODE_LIST_BOXES_RESPONSE,
            last, box_name, box_size, n_publishers, n_subscribers);
    return protocol_message;
}

char *publisher_message_protocol(const char message[1024]) {
    char *protocol_message = alloc_protocol_message();
    sprintf(protocol_message, "%d|%s", CODE_PUBLISHER_MESSAGE, message);
    return protocol_message;
}

char *subscriber_message_protocol(const char message[1024]) {
    char *protocol_message = alloc_protocol_message();
    sprintf(protocol_message, "%d|%s", CODE_SUBSCRIBER_MESSAGE, message);
    return protocol_message;
}

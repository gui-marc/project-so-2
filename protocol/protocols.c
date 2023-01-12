#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "betterassert.h"
#include "logging.h"
#include "protocols.h"

void client_create_pipes(
    const char client_named_pipe_path[NAMED_PIPE_PATH_SIZE]) {
    char client_reply_path[NAMED_PIPE_PATH_SIZE];
    sprintf(client_reply_path, "%s-reply", client_named_pipe_path);
    ALWAYS_ASSERT(unlink(client_named_pipe_path) == 0,
                  "Failed to cleanup/unlink client named pipe.");
    ALWAYS_ASSERT(unlink(client_reply_path) == 0,
                  "Failed to cleanup/unlink server named pipe.");

    ALWAYS_ASSERT(mkfifo(client_named_pipe_path, MKFIFO_PERMS) == 0,
                  "Failed to create client named pipe.");
    ALWAYS_ASSERT(mkfifo(client_reply_path, MKFIFO_PERMS) == 0,
                  "Failed to create server named pipe.");
}

struct named_pipes
client_open_pipes(const char client_named_pipe_path[NAMED_PIPE_PATH_SIZE]) {
    struct named_pipes np = {-1, -1};

    char client_reply_path[NAMED_PIPE_PATH_SIZE];
    sprintf(client_reply_path, "%s-reply", client_named_pipe_path);

    np.write_fd = open(client_named_pipe_path, O_WRONLY);
    ALWAYS_ASSERT(np.write_fd != -1, "Failed to open client named pipe")

    np.read_fd = open(client_reply_path, O_RDONLY);
    ALWAYS_ASSERT(np.read_fd != -1, "Failed to open client-reply named pipe");
    return np;
}

struct named_pipes
server_open_pipes(const char client_named_pipe_path[NAMED_PIPE_PATH_SIZE]) {
    struct named_pipes np = {-1, -1};

    char client_reply_path[NAMED_PIPE_PATH_SIZE];
    sprintf(client_reply_path, "%s-reply", client_named_pipe_path);

    np.read_fd = open(client_named_pipe_path, O_RDONLY);
    ALWAYS_ASSERT(np.write_fd != -1, "Failed to open client named pipe")

    np.write_fd = open(client_reply_path, O_WRONLY);
    ALWAYS_ASSERT(np.read_fd != -1, "Failed to open client-reply named pipe");
    return np;
}

void *request_protocol(uint8_t code, const char *client_named_pipe_path,
                       const char *box_name) {
    request_protocol_t *p = malloc(sizeof(request_protocol_t));
    p->base.code = code;
    strcpy(p->box_name, box_name);
    strcpy(p->client_named_pipe_path, client_named_pipe_path);
    return p;
}

void *response_protocol(uint8_t code, int32_t return_code,
                        const char *error_message) {
    response_protocol_t *p = malloc(sizeof(response_protocol_t));
    p->base.code = code;
    p->return_code = return_code;
    strcpy(p->error_message, error_message);
    return p;
}

void *register_publisher_protocol(const char *client_named_pipe_path,
                                  const char *box_name) {
    return request_protocol(CODE_REGISTER_PUBLISHER, client_named_pipe_path,
                            box_name);
}

void *register_subscriber_protocol(const char *client_named_pipe_path,
                                   const char *box_name) {
    return request_protocol(CODE_REGISTER_SUBSCRIBER, client_named_pipe_path,
                            box_name);
}

void *create_box_request_protocol(const char *client_named_pipe_path,
                                  const char *box_name) {
    return request_protocol(CODE_CREATE_BOX_REQUEST, client_named_pipe_path,
                            box_name);
}

void *create_box_response_protocol(int32_t return_code,
                                   const char *error_message) {
    return response_protocol(CODE_CREATE_BOX_RESPONSE, return_code,
                             error_message);
}

void *remove_box_request_protocol(const char *client_named_pipe_path,
                                  const char *box_name) {
    return request_protocol(CODE_REMOVE_BOX_REQUEST, client_named_pipe_path,
                            box_name);
}

void *remove_box_response_protocol(const int32_t return_code,
                                   const char *error_message) {
    return response_protocol(CODE_REMOVE_BOX_RESPONSE, return_code,
                             error_message);
}

void *list_boxes_request_protocol(const char *client_named_pipe_path) {
    list_boxes_request_protocol_t *p =
        malloc(sizeof(list_boxes_request_protocol_t));
    p->base.code = CODE_LIST_BOXES_REQUEST;
    strcpy(p->client_named_pipe_path, client_named_pipe_path);
    return p;
}

void *list_boxes_response_protocol(const uint8_t last, const char *box_name,
                                   const uint64_t box_size,
                                   const uint64_t n_publishers,
                                   const uint64_t n_subscribers) {
    list_boxes_response_protocol_t *p =
        malloc(sizeof(list_boxes_response_protocol_t));
    p->base.code = CODE_LIST_BOXES_RESPONSE;
    p->last = last;
    p->box_size = box_size;
    p->n_publishers = n_publishers;
    p->n_subscribers = n_subscribers;
    strcpy(p->box_name, box_name);
    return p;
}

void *publisher_message_protocol(const char *message) {
    message_protocol_t *p = malloc(sizeof(message_protocol_t));
    p->base.code = CODE_PUBLISHER_MESSAGE;
    strcpy(p->message, message);
    return p;
}

void *subscriber_message_protocol(const char *message) {
    message_protocol_t *p = malloc(sizeof(message_protocol_t));
    p->base.code = CODE_SUBSCRIBER_MESSAGE;
    strcpy(p->message, message);
    return p;
}

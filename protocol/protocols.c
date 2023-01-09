#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "betterassert.h"
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

void client_create_pipes(const char client_named_pipe_path[NAMED_PIPE_PATH_SIZE]) {
    char client_reply_path[NAMED_PIPE_PATH_SIZE];
    sprintf(client_reply_path, "%s-reply", client_named_pipe_path);
    ALWAYS_ASSERT(unlink(client_named_pipe_path) == 0, "Failed to cleanup/unlink client named pipe.");
    ALWAYS_ASSERT(unlink(client_reply_path) == 0, "Failed to cleanup/unlink server named pipe.");

    ALWAYS_ASSERT(mkfifo(client_named_pipe_path, MKFIFO_PERMS) == 0, "Failed to create client named pipe.");
    ALWAYS_ASSERT(mkfifo(client_reply_path, MKFIFO_PERMS) == 0, "Failed to create server named pipe.");

}

struct named_pipes client_open_pipes(const char client_named_pipe_path[NAMED_PIPE_PATH_SIZE]) {
    struct named_pipes np = {-1, -1};

    char client_reply_path[NAMED_PIPE_PATH_SIZE];
    sprintf(client_reply_path, "%s-reply", client_named_pipe_path);

    np.write_fd = open(client_named_pipe_path, O_WRONLY);
    ALWAYS_ASSERT(np.write_fd != -1, "Failed to open client named pipe")

    np.read_fd = open(client_reply_path, O_RDONLY);
    ALWAYS_ASSERT(np.read_fd != -1, "Failed to open client-reply named pipe");
    return np;
}

struct named_pipes server_open_pipes(const char client_named_pipe_path[NAMED_PIPE_PATH_SIZE]) {
    struct named_pipes np = {-1, -1};

    char client_reply_path[NAMED_PIPE_PATH_SIZE];
    sprintf(client_reply_path, "%s-reply", client_named_pipe_path);

    np.read_fd = open(client_named_pipe_path, O_RDONLY);
    ALWAYS_ASSERT(np.write_fd != -1, "Failed to open client named pipe")

    np.write_fd = open(client_reply_path, O_WRONLY);
    ALWAYS_ASSERT(np.read_fd != -1, "Failed to open client-reply named pipe");
    return np;
}

char *register_publisher_protocol(const char client_named_pipe_path[NAMED_PIPE_PATH_SIZE],
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

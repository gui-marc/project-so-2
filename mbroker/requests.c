#include "requests.h"
#include "betterassert.h"
#include "logging.h"
#include "operations.h"
#include "protocols.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void *listen_for_requests(void *queue) {
    while (true) {
        char *protocol = (char *)pcq_dequeue((pc_queue_t *)queue);
        u_int8_t code = (u_int8_t)protocol[0] - '0';
        parse_request(code, protocol);
        free(protocol);
    }
    return NULL;
}

void parse_request(u_int8_t request_code, void *protocol) {
    switch (request_code) {
    case REGISTER_PUBLISHER:
        register_publisher(protocol);
        break;
    case REGISTER_SUBSCRIBER:
        register_subscriber(protocol);
        break;
    case CREATE_BOX_REQUEST:
        create_box(protocol);
        break;
    case REMOVE_BOX_REQUEST:
        remove_box(protocol);
        break;
    case LIST_BOXES_REQUEST:
        list_boxes(protocol);
        break;
    default:
        WARN("invalid protocol code\n");
        break;
    }
}

void register_publisher(void *protocol) {
    (void)protocol;
    WARN("not implemented\n"); // Todo: implement me
}

void register_subscriber(void *protocol) {
    (void)protocol;
    WARN("not implemented\n"); // Todo: implement me
}

void create_box(void *protocol) {
    create_box_proto_t *request = (create_box_proto_t *)protocol;

    create_box_response_proto_t *response =
        calloc(1, sizeof(create_box_response_proto_t));
    ALWAYS_ASSERT(response != NULL, "Call to calloc failed.");
    response->return_code = REMOVE_BOX_RESPONSE;

    int pipe_fd = open_pipe(request->client_named_pipe_path);

    // tfs_lookup isn't exposed in headers,
    //  so use tfs_open for this check
    int fd = tfs_open(request->box_name, 0);
    if (fd != -1) {
        snprintf(response->error_msg, MSG_SIZE, ERR_BOX_ALREADY_EXISTS);
        tfs_close(fd);
        send_proto_string(pipe_fd, REMOVE_BOX_RESPONSE, response);
        return;
    }
    // Actually create the new file for the box
    fd = tfs_open(request->box_name, TFS_O_CREAT);
    if (fd == -1) {
        snprintf(response->error_msg, MSG_SIZE, ERR_BOX_CREATION);
    }
    send_proto_string(pipe_fd, REMOVE_BOX_RESPONSE, response);
}

void remove_box(void *protocol) {
    (void)protocol;
    WARN("not implemented\n"); // Todo: implement me
}

void list_boxes(void *protocol) {
    (void)protocol;
    WARN("not implemented\n"); // Todo: implement me
}
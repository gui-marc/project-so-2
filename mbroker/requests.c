#include "requests.h"
#include "betterassert.h"
#include "logging.h"
#include "operations.h"
#include "protocols.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

void *listen_for_requests(void *queue) {
    while (true) {
        queue_obj_t *obj = (queue_obj_t *)pcq_dequeue((pc_queue_t *)queue);
        parse_request(obj);
        free(obj->protocol);
        free(obj);
    }
    return NULL;
}

void parse_request(queue_obj_t *obj) {
    switch (obj->opcode) {
    case REGISTER_PUBLISHER:
        register_publisher(obj->protocol);
        break;
    case REGISTER_SUBSCRIBER:
        register_subscriber(obj->protocol);
        break;
    case CREATE_BOX_REQUEST:
        create_box(obj->protocol);
        break;
    case REMOVE_BOX_REQUEST:
        remove_box(obj->protocol);
        break;
    case LIST_BOXES_REQUEST:
        list_boxes(obj->protocol);
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

    // Metadata of the current inbox (Stores current publisher and subscribers)
    // Max filename from TFS is 40 (config.h), len(BOX_NAME_SIZE + ".meta") <
    // 40, should be okay
    char metadata_filename[BOX_NAME_SIZE] =
        sprintf("%s.meta", request->box_name);
    int metadata_fd = tfs_open(metadata_filename, TFS_O_CREAT);
    if (fd == -1 || metadata_fd == -1) {
        snprintf(response->error_msg, MSG_SIZE, ERR_BOX_CREATION);
        tfs_close(fd);
        tfs_close(metadata_fd);
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
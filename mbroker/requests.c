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
#include <string.h>
#include <unistd.h>

pthread_mutex_t tfs_ops = PTHREAD_MUTEX_INITIALIZER;

void *listen_for_requests(void *queue) {
    set_log_level(LOG_VERBOSE);
    while (true) {
        queue_obj_t *obj = (queue_obj_t *)pcq_dequeue((pc_queue_t *)queue);
        DEBUG("Dequeued object with code %u", obj->opcode);
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

    /*
    Metadata of the current inbox (Stores current publisher and subscribers)
     Max filename from TFS is 40 (config.h),
     (len(BOX_NAME_SIZE + ".meta\0") = 38) <
     40, should be okay

        char metadata_filename[MAX_FILE_NAME];
    snprintf(metadata_filename, MAX_FILE_NAME, "%s.pub", request->box_name);
    */

    register_pub_proto_t *request = (register_pub_proto_t *)protocol;
    // TODO: O_WRONLY | O_CREAT faz sentido sequer?
    int pipe_fd = open(request->client_named_pipe_path, O_WRONLY);
    ALWAYS_ASSERT(pipe_fd != -1, "Failed to open client named pipe")

    int fd = tfs_open(request->box_name, 0);
    char metadata_filename[MAX_FILE_NAME];

    snprintf(metadata_filename, MAX_FILE_NAME, "%s.meta", request->box_name);
    int metadata_fd = tfs_open(metadata_filename, 0);

    if (fd == -1 || metadata_fd == -1) {
        close(pipe_fd); // Supposedly, this is equivalent to sending EOF.
        tfs_close(fd);
        tfs_close(metadata_fd);
        return;
    }
    // Check if there's a publisher already
    char cur_publisher[NPIPE_PATH_SIZE];
    tfs_read(metadata_fd, &cur_publisher, NPIPE_PATH_SIZE);
    char *null_publisher = calloc(1, NPIPE_PATH_SIZE);
    // There's a publisher already - exit
    if (!STR_MATCH(cur_publisher, null_publisher)) {
        close(pipe_fd);
        tfs_close(fd);
        tfs_close(metadata_fd);
        return;
    }
    // There's no publisher, continue
    // tfs_write(metadata_fd, )
    // TODO RG: look here

    // if (STR_MATCH(cur_publisher))
    /*TODO these checks:
    if tfs_write fails;
    if a publisher already exists;

    */
}

void register_subscriber(void *protocol) {
    (void)protocol;
    WARN("not implemented\n"); // Todo: implement me
}

void create_box(void *protocol) {
    create_box_proto_t *request = (create_box_proto_t *)protocol;

    create_box_response_proto_t *response =
        calloc(1, sizeof(create_box_response_proto_t));
    ALWAYS_ASSERT(
        response != NULL,
        "Call to calloc failed."); // TODO: wrap calloc instead of this
    response->return_code = REMOVE_BOX_RESPONSE;

    int pipe_fd = open_pipe(request->client_named_pipe_path, O_WRONLY);

    // Check if the box already exists.
    // Send error and quit, if so.
    pthread_mutex_lock(&tfs_ops);

    // On a real filesystem, we could probably open this file with a
    // `CREATE_IF_NOT_EXISTS`-equivalent flag. Since TFS doesn't have that
    // feature (and we can't use tfs_lookup here), we have to call `tfs_open`
    // twice
    int fd = tfs_open(request->box_name, 0);
    if (fd != -1) {
        snprintf(response->error_msg, MSG_SIZE, ERR_BOX_ALREADY_EXISTS);
        tfs_close(fd);
        pthread_mutex_unlock(&tfs_ops);
        send_proto_string(pipe_fd, REMOVE_BOX_RESPONSE, response);
        close(pipe_fd);
        return;
    }
    // Create the new file for the box
    fd = tfs_open(request->box_name, TFS_O_CREAT | TFS_O_TRUNC);

    if (fd == -1) {
        snprintf(response->error_msg, MSG_SIZE, ERR_BOX_CREATION);
        tfs_close(fd);
    }
    pthread_mutex_unlock(&tfs_ops);
    send_proto_string(pipe_fd, REMOVE_BOX_RESPONSE, response);
    close(pipe_fd);
    return;
}

void remove_box(void *protocol) {
    remove_box_proto_t * request = (remove_box_proto_t*) protocol;
    ALWAYS_ASSERT(tfs_unlink(request->box_name) == 0, "Failed to remove box");
}

void list_boxes(void *protocol) {
    list_boxes_request_proto_t * request = (list_boxes_request_proto_t*) protocol;


}
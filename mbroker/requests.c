#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "betterassert.h"
#include "logging.h"
#include "mbroker.h"
#include "operations.h"
#include "protocols.h"
#include "requests.h"

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
    register_pub_proto_t *request = (register_pub_proto_t *)protocol;
    // TODO: O_WRONLY | O_CREAT faz sentido sequer?
    int pipe_fd = open(request->client_named_pipe_path, O_RDONLY);
    ALWAYS_ASSERT(pipe_fd != -1, "Failed to open client named pipe")

    int fd = tfs_open(request->box_name, TFS_O_APPEND);
    box_metadata_t *box = box_holder_find_box(&box_holder, request->box_name);
    // If we couldn't open the box for whatever reason,
    // exit.
    if (fd == -1 || box == NULL) {
        close(pipe_fd); // Supposedly, this is equivalent to sending EOF.
        tfs_close(fd);
        return;
    }

    pthread_mutex_lock(&box->has_publisher_lock);
    if (box->has_publisher == true) {
        close(pipe_fd);
        tfs_close(fd);
        pthread_mutex_unlock(&box->has_publisher_lock);
        return;
    }
    box->has_publisher = true;
    pthread_mutex_unlock(&box->has_publisher_lock);
    box->publisher_idx = pthread_self();

    uint8_t op;
    publisher_msg_proto_t *pub_msg;
    size_t written = 0;
    ssize_t wrote;
    size_t msg_len = 0;

    while (0) { // FIXME: Should check a variable! (Maybe create an array that a
                // signal handler changes this thread's variable)
        op = recv_opcode(pipe_fd);
        if (op != PUBLISHER_MESSAGE) {
            DEBUG("Received invalid opcode from publisher for box '%s'",
                  request->box_name);
            // Didn't expect this message: quit
            break;
        }
        pub_msg = (publisher_msg_proto_t *)parse_protocol(pipe_fd, op);
        DEBUG("Received this message from publisher of '%s': '%s'",
              request->box_name, pub_msg->msg);
        msg_len = strlen(pub_msg->msg) + 1;
        wrote = tfs_write(fd, pub_msg->msg,
                          strlen(pub_msg->msg) + 1); //+1 to include \0
        if (wrote < 0) {
            // ... error
            DEBUG("Error occurred in tfs_write publisher msg")
            break;
        } else {
            written = (size_t)wrote;
        }

        pthread_mutex_lock(&box->total_message_size_lock);
        box->total_message_size += msg_len;
        pthread_mutex_unlock(&box->total_message_size_lock);
        pthread_cond_broadcast(&box->read_condvar);
        free(pub_msg);
        if (written != msg_len) {
            // We don't explicitly disallow new publishers to connect after this
            // one quits, But subsequent publishers will all fail here and won't
            // write anything else.
            DEBUG("Couldn't write entire message to TFS. Quitting.")
            break;
        }
    }

    DEBUG("Cleaning up thread for publisher of '%s'", request->box_name);
    pthread_mutex_lock(&box->has_publisher_lock);
    box->has_publisher = false;
    pthread_mutex_unlock(&box->has_publisher_lock);
    close(pipe_fd);
    tfs_close(fd);
    DEBUG("Thread for publisher of '%s' finished", request->box_name);
    return;
}

void register_subscriber(void *protocol) {
    register_sub_proto_t *request = (register_sub_proto_t *)protocol;
    int pipe_fd = open(request->client_named_pipe_path, O_WRONLY);
    ALWAYS_ASSERT(pipe_fd != -1,
                  "An error ocurred while opening subscriber pipe %s",
                  request->client_named_pipe_path);
    box_metadata_t *box = box_holder_find_box(&box_holder, request->box_name);
    if (box == NULL) {
        // free(response_proto);
        close(pipe_fd);
    }
    int fd = tfs_open(request->box_name, 0);
    if (fd == -1) {
        close(pipe_fd);
    }

    // Todo: change subscribers array (it is even needed?)
    pthread_mutex_lock(&box->subscribers_count_lock);
    box->subscribers_count++;
    pthread_mutex_unlock(&box->subscribers_count_lock);

    size_t bytes_read = 0;
    size_t to_write = 0;
    char *buf_og = calloc(MSG_SIZE, sizeof(char));
    char *msg_buf;
    char *tmp_msg = calloc(MSG_SIZE, sizeof(char));
    basic_msg_proto_t *msg = NULL;
    // This first while loop only closes with a signal or (todo) another condvar
    while (true) {
        // Waits for a new message
        pthread_mutex_lock(&box->total_message_size_lock);
        while (box->total_message_size > bytes_read) {
            pthread_cond_wait(&box->read_condvar,
                              &box->total_message_size_lock);
        }
        // Read remaining bytes
        msg_buf = buf_og;
        tfs_read(fd, msg_buf, box->total_message_size - bytes_read);
        while (box->total_message_size > bytes_read) {
            strcpy(tmp_msg, msg_buf);
            to_write = strlen(msg_buf) + 1;
            msg = message_proto(tmp_msg);
            send_proto_string(pipe_fd, SUBSCRIBER_MESSAGE, msg);
            msg_buf += to_write;
            bytes_read += to_write;
        }

        // acho que aqui é uma condição para "fechar" o subscriber
        // podiamos meter uma outra condvar para fechar o subscriber quando
        // fechasse a box faz sentido?
        pthread_mutex_unlock(&box->total_message_size_lock);
    }
    DEBUG("Quitting subscriber, cleaning up...");

    pthread_mutex_lock(&box->subscribers_count_lock);
    box->subscribers_count--;
    pthread_mutex_unlock(&box->subscribers_count_lock);
}

void create_box(void *protocol) {
    create_box_proto_t *request = (create_box_proto_t *)protocol;
    // TODO: Free response!
    create_box_response_proto_t *response =
        calloc(1, sizeof(create_box_response_proto_t));
    ALWAYS_ASSERT(
        response != NULL,
        "Call to calloc failed."); // TODO: wrap calloc instead of this
    response->return_code = REMOVE_BOX_RESPONSE;

    int pipe_fd = open_pipe(request->client_named_pipe_path, O_WRONLY);

    box_metadata_t *box = box_holder_find_box(&box_holder, request->box_name);

    if (box != NULL) {
        snprintf(response->error_msg, MSG_SIZE, ERR_BOX_ALREADY_EXISTS);
        send_proto_string(pipe_fd, REMOVE_BOX_RESPONSE, response);
        close(pipe_fd);
        return;
    }
    // Create the new file for the box
    int fd = tfs_open(request->box_name, TFS_O_CREAT | TFS_O_TRUNC);

    if (fd == -1) {
        snprintf(response->error_msg, MSG_SIZE, ERR_BOX_CREATION);
        tfs_close(fd);
    }
    box_metadata_t *new_box =
        box_metadata_create(request->box_name, max_sessions);
    box_holder_insert(&box_holder, new_box);
    send_proto_string(pipe_fd, CREATE_BOX_RESPONSE, response);
    close(pipe_fd);
    return;
}

void remove_box(void *protocol) {
    remove_box_proto_t *request = (remove_box_proto_t *)protocol;

    // Removes box from tfs
    ALWAYS_ASSERT(tfs_unlink(request->box_name) == 0, "Failed to remove box");

    int wx = open_pipe(request->client_named_pipe_path, O_CREAT | O_WRONLY);

    if (box_holder_remove(&box_holder, request->box_name) == 0) {
        // Box was removed successfully
        response_proto_t *res = response_proto(0, "\0");
        send_proto_string(wx, REMOVE_BOX_RESPONSE, res);
        free(res);
    } else {
        // Error while removing box
        char *error_msg = calloc(MSG_SIZE, sizeof(char));
        sprintf(error_msg, "Failed to remove box with name: %s",
                request->box_name);
        response_proto_t *res = response_proto(0, error_msg);
        send_proto_string(wx, REMOVE_BOX_RESPONSE, res);
        free(error_msg);
        free(res);
    }

    // Todo: close all related subscribers and publishers

    close(wx);
}

void list_boxes(void *protocol) {
    list_boxes_request_proto_t *request =
        (list_boxes_request_proto_t *)protocol;

    int wr = open_pipe(request->client_named_pipe_path, O_CREAT | O_WRONLY);

    pthread_mutex_lock(&box_holder.lock);

    // Send a response for each box
    for (size_t i = 0; i < box_holder.current_size; i++) {
        // send each request
        box_metadata_t *box = box_holder.boxes[i];
        uint8_t last = 0;
        if (i == box_holder.current_size - 1) {
            last = 1;
        }
        pthread_mutex_lock(&box->total_message_size_lock);
        pthread_mutex_lock(&box->has_publisher_lock);
        pthread_mutex_lock(&box->subscribers_count_lock);
        list_boxes_response_proto_t *res = list_boxes_response_proto(
            last, box->name, box->total_message_size, box->has_publisher,
            box->subscribers_count);
        pthread_mutex_unlock(&box->total_message_size_lock);
        pthread_mutex_unlock(&box->has_publisher_lock);
        pthread_mutex_unlock(&box->subscribers_count_lock);

        send_proto_string(wr, LIST_BOXES_RESPONSE, res);
    }

    // No boxes
    if (box_holder.current_size == 0) {
        char *msg = calloc(BOX_NAME_SIZE, sizeof(char)); // String with 32 '\0's
        list_boxes_response_proto_t *res =
            list_boxes_response_proto(1, msg, 0, false, 0);
        send_proto_string(wr, LIST_BOXES_RESPONSE, res);
        free(msg);
    }

    pthread_mutex_unlock(&box_holder.lock);

    close(wr);
}
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "betterassert.h"
#include "logging.h"
#include "operations.h"
#include "protocols.h"
#include "requests.h"
#include "utils.h"

uint8_t sigpipe_called = 0;

void sigpipe_handler() {
    DEBUG("Caught SIGINT! Exiting.");
    sigpipe_called = 1;
}

void *listen_for_requests(void *args) {
    void **real_args = (void **)args;
    pc_queue_t *queue = (pc_queue_t *)real_args[0];
    box_holder_t *box_holder = (box_holder_t *)real_args[1];
    set_log_level(LOG_VERBOSE);
    while (true) {
        queue_obj_t *obj = (queue_obj_t *)pcq_dequeue((pc_queue_t *)queue);
        DEBUG("Dequeued object with code %u", obj->opcode);
        parse_request(obj, box_holder);
        gg_free((void **)&obj->protocol);
        gg_free((void **)&obj);
    }
    return NULL;
}

void parse_request(queue_obj_t *obj, box_holder_t *box_holder) {
    DEBUG("parse_request...");
    switch (obj->opcode) {
    case REGISTER_PUBLISHER:
        register_publisher(obj->protocol, box_holder);
        break;
    case REGISTER_SUBSCRIBER:
        register_subscriber(obj->protocol, box_holder);
        break;
    case CREATE_BOX_REQUEST:
        create_box(obj->protocol, box_holder);
        break;
    case REMOVE_BOX_REQUEST:
        remove_box(obj->protocol, box_holder);
        break;
    case LIST_BOXES_REQUEST:
        list_boxes(obj->protocol, box_holder);
        break;
    // These are not valid opcodes to receive at this stage.
    case REMOVE_BOX_RESPONSE:
    case LIST_BOXES_RESPONSE:
    case PUBLISHER_MESSAGE:
    case SUBSCRIBER_MESSAGE:
    case CREATE_BOX_RESPONSE:
    default:
        WARN("invalid protocol code\n");
        break;
    }
}

void register_publisher(void *protocol, box_holder_t *box_holder) {
    register_pub_proto_t *request = (register_pub_proto_t *)protocol;
    DEBUG("Starting register_publisher for client_pipe '%s'",
          request->client_named_pipe_path);
    int pipe_fd = open(request->client_named_pipe_path, O_RDONLY);
    ALWAYS_ASSERT(pipe_fd != -1, "Failed to open client named pipe")

    char *box_path __attribute__((cleanup(str_cleanup))) =
        calloc(1, sizeof(char) * (BOX_NAME_SIZE + 1));
    strcpy(box_path, "/");
    strncpy(box_path + 1, request->box_name, BOX_NAME_SIZE);

    int fd = tfs_open(box_path, TFS_O_APPEND);
    box_metadata_t *box = box_holder_find_box(box_holder, request->box_name);
    DEBUG("Checking if box '%s' exists...", request->box_name);
    // If we couldn't open the box for whatever reason,
    // exit.
    if (fd == -1 || box == NULL) {
        if (box == NULL) {
            DEBUG("Opening metadata of box '%s' failed.", request->box_name);
        } else if (fd == -1) {
            DEBUG("tfs_open of file '%s' failed.", box_path);
        }
        DEBUG("Box does not exist, quitting.")
        gg_close(pipe_fd); // Equivalent to sending EOF.
        tfs_close(fd);
        return;
    }
    DEBUG("Passed box existence check.");

    pthread_mutex_lock(&box->has_publisher_lock);
    if (box->has_publisher == true) {
        DEBUG("Box already has a publisher. Quitting.");
        gg_close(pipe_fd);
        tfs_close(fd);
        pthread_mutex_unlock(&box->has_publisher_lock);
        return;
    }
    box->has_publisher = true;
    pthread_mutex_unlock(&box->has_publisher_lock);
    box->publisher_idx = pthread_self();

    // Redefine SIGINT treatment
    signal(SIGPIPE, sigpipe_handler);

    uint8_t op;
    publisher_msg_proto_t *pub_msg;
    size_t written = 0;
    ssize_t wrote;
    size_t msg_len = 0;
    DEBUG("Entering publisher loop.");
    while (sigpipe_called ==
           0) { // FIXME: Should check a variable! (Maybe create an array
                // that a signal handler changes this thread's variable)
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
    gg_close(pipe_fd);
    tfs_close(fd);
    DEBUG("Thread for publisher of '%s' finished", request->box_name);
    return;
}

void register_subscriber(void *protocol, box_holder_t *box_holder) {
    DEBUG("register_subscriber started...");
    register_sub_proto_t *request = (register_sub_proto_t *)protocol;
    int pipe_fd = open_pipe(request->client_named_pipe_path, O_WRONLY);
    box_metadata_t *box = box_holder_find_box(box_holder, request->box_name);
    if (box == NULL) {
        DEBUG("Failed to find metadata box for %s", request->box_name);
        gg_close(pipe_fd);
        return;
    }
    // Create the new file for the box
    // TFS's max filename is 40, so we're good
    char *box_path __attribute__((cleanup(str_cleanup))) =
        calloc(1, sizeof(char) * (BOX_NAME_SIZE + 1));
    strcpy(box_path, "/");
    strncpy(box_path + 1, request->box_name, BOX_NAME_SIZE);
    int fd = tfs_open(box_path, 0);
    if (fd == -1) {
        DEBUG("Failed to open TFS file %s", box_path);
        gg_close(pipe_fd);
        return;
    }

    // Todo: change subscribers array (it is even needed?)
    pthread_mutex_lock(&box->subscribers_count_lock);
    box->subscribers_count++;
    pthread_mutex_unlock(&box->subscribers_count_lock);

    // Redefine SIGINT treatment
    signal(SIGPIPE, sigpipe_handler);

    bool must_break = false;
    size_t bytes_read = 0;
    size_t to_write = 0;
    char *buf_og __attribute__((cleanup(str_cleanup))) =
        calloc(MSG_SIZE, sizeof(char));
    char *msg_buf;
    char *tmp_msg __attribute__((cleanup(str_cleanup))) =
        calloc(MSG_SIZE, sizeof(char));
    basic_msg_proto_t *msg = NULL;
    // This first while loop only closes with a signal or (todo) another condvar
    DEBUG("Entering subscriber loop.");
    while (sigpipe_called == 0 && !must_break) {
        // Waits for a new message
        pthread_mutex_lock(&box->total_message_size_lock);
        while (box->total_message_size <= bytes_read) {
            DEBUG("Waiting for read condition with current size: %lu of %lu",
                  bytes_read, box->total_message_size);
            pthread_cond_wait(&box->read_condvar,
                              &box->total_message_size_lock);
        }
        // Read remaining bytes
        DEBUG("Started reading messages");
        msg_buf = buf_og;
        tfs_read(fd, msg_buf, box->total_message_size - bytes_read);
        while (box->total_message_size > bytes_read) {
            DEBUG("Entering second while loop");
            strcpy(tmp_msg, msg_buf);
            to_write = strlen(msg_buf) + 1;
            msg = message_proto(tmp_msg);
            DEBUG("Sending message: %s", msg->msg);
            if (send_proto_string(pipe_fd, SUBSCRIBER_MESSAGE, msg) == -1) {
                // Pipe was already closed, so it must break
                must_break = true;
            }
            msg_buf += to_write;
            bytes_read += to_write;
            DEBUG("Bytes read is now: %lu", bytes_read);
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

// FIXME: isto dá segmentation fault a remover uma caixa que não existe
void create_box(void *protocol, box_holder_t *box_holder) {
    DEBUG("create_box started...");
    create_box_proto_t *request = (create_box_proto_t *)protocol;

    DEBUG("opening client named pipe %s", request->client_named_pipe_path);
    int pipe_fd = open_pipe(request->client_named_pipe_path, O_WRONLY);

    box_metadata_t *box = box_holder_find_box(box_holder, request->box_name);

    if (box != NULL) {
        DEBUG("Box already exists - quitting.");
        response_proto_t *res
            __attribute__((cleanup(response_proto_t_cleanup))) =
                response_proto(-1, ERR_BOX_ALREADY_EXISTS);
        send_proto_string(pipe_fd, REMOVE_BOX_RESPONSE, res);
        gg_close(pipe_fd);
        return;
    }
    DEBUG("box_name = '%s'", request->box_name);
    // Create the new file for the box
    // TFS's max filename is 40, so we're good
    char *box_path __attribute__((cleanup(str_cleanup))) =
        calloc(1, sizeof(char) * (BOX_NAME_SIZE + 1));
    strcpy(box_path, "/");
    strncpy(box_path + 1, request->box_name, BOX_NAME_SIZE);

    DEBUG("Saving box at path: '%s'", box_path);
    int fd = tfs_open(box_path, TFS_O_CREAT | TFS_O_TRUNC);

    if (fd == -1) {
        DEBUG("Error while saving box in TFS");
        response_proto_t *res
            __attribute__((cleanup(response_proto_t_cleanup))) =
                response_proto(-1, ERR_BOX_CREATION);
        send_proto_string(pipe_fd, CREATE_BOX_RESPONSE, res);
        tfs_close(fd);
        gg_close(pipe_fd);
        return;
    }
    DEBUG("Finished saving box");

    box_metadata_t *new_box = box_metadata_create(request->box_name);

    box_holder_insert(box_holder, new_box);

    DEBUG("Created box successfully");
    response_proto_t *res __attribute__((cleanup(response_proto_t_cleanup))) =
        response_proto(0, "\0");
    send_proto_string(pipe_fd, CREATE_BOX_RESPONSE, res);
    gg_close(pipe_fd);
    return;
}

void remove_box(void *protocol, box_holder_t *box_holder) {
    DEBUG("remove_box started...");
    remove_box_proto_t *request = (remove_box_proto_t *)protocol;
    bool remove_failed = false;

    char *box_path __attribute__((cleanup(str_cleanup))) =
        calloc(1, sizeof(char) * (BOX_NAME_SIZE + 1));
    strcpy(box_path, "/");
    strncpy(box_path + 1, request->box_name, BOX_NAME_SIZE);

    int wx = open_pipe(request->client_named_pipe_path, O_CREAT | O_WRONLY);

    box_metadata_t *box = box_holder_find_box(box_holder, request->box_name);
    if (box == NULL) {
        response_proto_t *res
            __attribute__((cleanup(response_proto_t_cleanup))) =
                response_proto(-1, "Box with that name was not found");
        send_proto_string(wx, REMOVE_BOX_RESPONSE, res);
        gg_close(wx);
        return;
    }

    // Removes box from tfs
    DEBUG("Removing box '%s' from tfs", box_path);
    ALWAYS_ASSERT(tfs_unlink(box_path) == 0, "Failed to remove box");
    DEBUG("Finished removing box");

    // Todo: close all related subscribers and publishers
    if (box_holder_remove(box_holder, request->box_name) == 0 &&
        remove_failed == false) {
        DEBUG("Success removing box '%s'", request->box_name);
        // Box was removed successfully
        response_proto_t *res
            __attribute__((cleanup(response_proto_t_cleanup))) =
                response_proto(0, "\0");
        send_proto_string(wx, REMOVE_BOX_RESPONSE, res);
    } else {
        DEBUG("Box removal failed!");
        // Error while removing box
        char *error_msg __attribute__((cleanup(str_cleanup))) =
            calloc(MSG_SIZE, sizeof(char));
        snprintf(error_msg, MSG_SIZE, "Failed to remove box with name: %s",
                 request->box_name);

        // Removed attribute cleanup
        response_proto_t *res
            __attribute__((cleanup(response_proto_t_cleanup))) =
                response_proto(-1, error_msg);

        send_proto_string(wx, REMOVE_BOX_RESPONSE, res);
    }
    gg_close(wx);
    return;
}

void list_boxes(void *protocol, box_holder_t *box_holder) {
    DEBUG("list_boxes started...");
    list_boxes_request_proto_t *request =
        (list_boxes_request_proto_t *)protocol;

    int wr = open_pipe(request->client_named_pipe_path, O_WRONLY);

    pthread_mutex_lock(&box_holder->lock);

    // Send a response for each box
    for (size_t i = 0; i < box_holder->current_size; i++) {
        // send each request
        box_metadata_t *box = box_holder->boxes[i];
        uint8_t last = 0;
        if (i == box_holder->current_size - 1) {
            last = 1;
        }
        pthread_mutex_lock(&box->total_message_size_lock);
        pthread_mutex_lock(&box->has_publisher_lock);
        pthread_mutex_lock(&box->subscribers_count_lock);
        list_boxes_response_proto_t *res
            __attribute__((cleanup(ls_boxes_resp_proto_cleanup))) =
                list_boxes_response_proto(
                    last, box->name, box->total_message_size,
                    box->has_publisher, box->subscribers_count);
        pthread_mutex_unlock(&box->total_message_size_lock);
        pthread_mutex_unlock(&box->has_publisher_lock);
        pthread_mutex_unlock(&box->subscribers_count_lock);

        DEBUG("Sending box %s", box->name);
        send_proto_string(wr, LIST_BOXES_RESPONSE, res);
    }

    // No boxes
    if (box_holder->current_size == 0) {
        char *msg __attribute__((cleanup(str_cleanup))) =
            calloc(BOX_NAME_SIZE, sizeof(char)); // String with 32 '\0's
        list_boxes_response_proto_t *res
            __attribute__((cleanup(ls_boxes_resp_proto_cleanup))) =
                list_boxes_response_proto(1, msg, 0, false, 0);
        send_proto_string(wr, LIST_BOXES_RESPONSE, res);
    }

    pthread_mutex_unlock(&box_holder->lock);

    gg_close(wr);
}
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

uint8_t sigusr_called = 0;

/**
 * @brief Handles the SIGUSR1 signal, sent by the mbroker when the main thread
 * is closing. After that, the thread will end.
 */
void sigusr_handler() { sigusr_called = 1; }

void *listen_for_requests(void *args) {
    void **real_args = (void **)args;
    pc_queue_t *queue = (pc_queue_t *)real_args[0];
    box_holder_t *box_holder = (box_holder_t *)real_args[1];

    // Signal to finish the thread
    signal(SIGUSR1, sigusr_handler);

    // While the thread is not ended
    while (sigusr_called == 0) {
        // Tries to read a request from the pcq
        queue_obj_t *obj = (queue_obj_t *)pcq_dequeue((pc_queue_t *)queue);
        parse_request(obj, box_holder);
        free(obj->protocol);
        free(obj);
    }

    return NULL;
}

void parse_request(queue_obj_t *obj, box_holder_t *box_holder) {
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

    // Opens the pipe to send information to the publisher client
    int pipe_fd = open(request->client_named_pipe_path, O_RDONLY);
    if (pipe_fd == -1) {
        WARN("Failed to open client named pipe");
        return;
    }

    // Represents a path to the box in the TFS
    char *box_path __attribute__((cleanup(str_cleanup))) =
        calloc(1, sizeof(char) * (BOX_NAME_SIZE + 1));
    strcpy(box_path, "/");
    strncpy(box_path + 1, request->box_name, BOX_NAME_SIZE);

    // Opens the box
    int fd = tfs_open(box_path, TFS_O_APPEND);
    box_metadata_t *box = box_holder_find_box(box_holder, request->box_name);

    // Ends the program if the box was not found or there was an error while
    // opening it
    if (fd == -1 || box == NULL) {
        if (box == NULL) {
            WARN("Box '%s' doesn't exist", request->box_name);
        } else if (fd == -1) {
            WARN("Box '%s' failed to open", request->box_name);
        }
        close(pipe_fd);
        tfs_close(fd);
        return;
    }

    // Verifies if is the only publisher looking at that box
    pthread_mutex_lock(&box->has_publisher_lock);
    if (box->has_publisher == true) {
        WARN("Box already has a publisher. Quitting.");
        close(pipe_fd);
        tfs_close(fd);
        pthread_mutex_unlock(&box->has_publisher_lock);
        return;
    }

    // Sets the box metadata
    box->has_publisher = true;
    pthread_mutex_unlock(&box->has_publisher_lock);
    box->publisher_idx = pthread_self();

    // Redefine SIGINT treatment
    signal(SIGPIPE, SIG_IGN);

    uint8_t op;
    publisher_msg_proto_t *pub_msg;
    size_t written = 0;
    ssize_t wrote;
    size_t msg_len = 0;
    while (true) {
        // This while loop ends when the pipe is closed (the user closed the
        // publisher client)

        op = recv_opcode(pipe_fd);
        if (op != PUBLISHER_MESSAGE) {
            WARN("Received invalid opcode from publisher for box '%s'",
                 request->box_name);
            // Didn't expect this message: quit
            break;
        }

        pub_msg = (publisher_msg_proto_t *)parse_protocol(pipe_fd, op);
        msg_len = strlen(pub_msg->msg) + 1;
        wrote = tfs_write(fd, pub_msg->msg,
                          strlen(pub_msg->msg) + 1); //+1 to include \0
        if (wrote < 0) {
            WARN("Error occurred in tfs_write publisher msg")
            break;
        } else {
            written = (size_t)wrote;
        }

        // Updates the message size so the subscribers can read
        pthread_mutex_lock(&box->total_message_size_lock);
        box->total_message_size += msg_len;
        pthread_mutex_unlock(&box->total_message_size_lock);

        // Let subscriber threads read the new message
        pthread_cond_broadcast(&box->read_condvar);

        if (written != msg_len) {
            // We don't explicitly disallow new publishers to connect after this
            // one quits, But subsequent publishers will all fail here and won't
            // write anything else.
            WARN("Couldn't write entire message to TFS. Quitting.")
            break;
        }
    }

    // Clean the thread
    pthread_mutex_lock(&box->has_publisher_lock);
    box->has_publisher = false;
    pthread_mutex_unlock(&box->has_publisher_lock);
    close(pipe_fd);
    tfs_close(fd);
    return;
}

void register_subscriber(void *protocol, box_holder_t *box_holder) {
    register_sub_proto_t *request = (register_sub_proto_t *)protocol;

    // Opens the pipe to send messages to the client
    int pipe_fd = open_pipe(request->client_named_pipe_path, O_WRONLY);
    box_metadata_t *box = box_holder_find_box(box_holder, request->box_name);

    // No box was found with the given box_name
    if (box == NULL) {
        WARN("Failed to find metadata box for %s", request->box_name);
        close(pipe_fd);
        return;
    }

    // Creates the path to the box in the TFS
    char *box_path __attribute__((cleanup(str_cleanup))) =
        calloc(1, sizeof(char) * (BOX_NAME_SIZE + 1));
    strcpy(box_path, "/");
    strncpy(box_path + 1, request->box_name, BOX_NAME_SIZE);

    // Opens the box in the TFS
    int fd = tfs_open(box_path, 0);
    if (fd == -1) {
        WARN("Failed to open TFS file %s", box_path);
        close(pipe_fd);
        return;
    }

    // Updates the subscribers count in the box metadata
    // Is used by the manager when listing boxes
    pthread_mutex_lock(&box->subscribers_count_lock);
    box->subscribers_count++;
    pthread_mutex_unlock(&box->subscribers_count_lock);

    // Redefine SIGINT treatment
    signal(SIGPIPE, SIG_IGN);

    bool must_break = false;
    size_t bytes_read = 0;
    size_t to_write = 0;
    char *buf_og __attribute__((cleanup(str_cleanup))) =
        calloc(MSG_SIZE, sizeof(char));
    char *msg_buf;
    char *tmp_msg __attribute__((cleanup(str_cleanup))) =
        calloc(MSG_SIZE, sizeof(char));
    basic_msg_proto_t *msg = NULL;
    // This first while loop only closes with a signal
    while (must_break == 0) {
        // Waits for a new message
        pthread_mutex_lock(&box->total_message_size_lock);
        while (box->total_message_size <= bytes_read) {
            // Waits for the message size to change
            pthread_cond_wait(&box->read_condvar,
                              &box->total_message_size_lock);
        }

        // Read remaining bytes
        msg_buf = buf_og;
        tfs_read(fd, msg_buf, box->total_message_size - bytes_read);
        while (box->total_message_size > bytes_read) {
            strcpy(tmp_msg, msg_buf);
            to_write = strlen(msg_buf) + 1; // + 1 to include \0 at the end
            msg = message_proto(tmp_msg);
            if (send_proto_string(pipe_fd, SUBSCRIBER_MESSAGE, msg) == -1) {
                // Pipe was already closed, so it must break
                must_break = true;
            }
            msg_buf += to_write; // offset the buffer to the next read
            bytes_read += to_write;
        }

        pthread_mutex_unlock(&box->total_message_size_lock);
    }

    // Decrease the subscribers count
    pthread_mutex_lock(&box->subscribers_count_lock);
    box->subscribers_count--;
    pthread_mutex_unlock(&box->subscribers_count_lock);
}

void create_box(void *protocol, box_holder_t *box_holder) {
    create_box_proto_t *request = (create_box_proto_t *)protocol;

    // Opens the pipe to send information to the client
    int pipe_fd = open_pipe(request->client_named_pipe_path, O_WRONLY);

    // Search for the box in the box_holder
    box_metadata_t *box = box_holder_find_box(box_holder, request->box_name);

    // If there is already a box with that box_name
    if (box != NULL) {
        WARN("Box already exists - quitting.");
        response_proto_t *res
            __attribute__((cleanup(response_proto_t_cleanup))) =
                response_proto(-1, ERR_BOX_ALREADY_EXISTS);
        send_proto_string(pipe_fd, REMOVE_BOX_RESPONSE, res);
        close(pipe_fd);
        return;
    }

    // Creates the path to the box in the TFS
    char *box_path __attribute__((cleanup(str_cleanup))) =
        calloc(1, sizeof(char) * (BOX_NAME_SIZE + 1));
    strcpy(box_path, "/");
    strncpy(box_path + 1, request->box_name, BOX_NAME_SIZE);

    // Saves the box in the TFS
    int fd = tfs_open(box_path, TFS_O_CREAT | TFS_O_TRUNC);

    // Failed to save box in the TFS
    if (fd == -1) {
        WARN("Error while saving box in TFS");
        response_proto_t *res
            __attribute__((cleanup(response_proto_t_cleanup))) =
                response_proto(-1, ERR_BOX_CREATION);
        send_proto_string(pipe_fd, CREATE_BOX_RESPONSE, res);
        tfs_close(fd);
        close(pipe_fd);
        return;
    }

    // Creates a metadata to store in the mbroker
    box_metadata_t *new_box = box_metadata_create(request->box_name);

    // Stores the new metadata in the box_holder (to be used by another
    // functionalities)
    box_holder_insert(box_holder, new_box);

    // Sends the OK response to the client
    response_proto_t *res __attribute__((cleanup(response_proto_t_cleanup))) =
        response_proto(0, "\0");
    send_proto_string(pipe_fd, CREATE_BOX_RESPONSE, res);
    close(pipe_fd);
    return;
}

void remove_box(void *protocol, box_holder_t *box_holder) {
    remove_box_proto_t *request = (remove_box_proto_t *)protocol;
    bool remove_failed = false;

    // Creates the path to the box in the TFS
    char *box_path __attribute__((cleanup(str_cleanup))) =
        calloc(1, sizeof(char) * (BOX_NAME_SIZE + 1));
    strcpy(box_path, "/");
    strncpy(box_path + 1, request->box_name, BOX_NAME_SIZE);

    int wx = open_pipe(request->client_named_pipe_path, O_CREAT | O_WRONLY);

    box_metadata_t *box = box_holder_find_box(box_holder, request->box_name);
    if (box == NULL) {
        response_proto_t *res =
            response_proto(-1, "Box with that name was not found");
        send_proto_string(wx, REMOVE_BOX_RESPONSE, res);
        free(res);
        close(wx);
        return;
    }

    // Removes box from tfs
    ALWAYS_ASSERT(tfs_unlink(box_path) == 0, "Failed to remove box");

    if (box_holder_remove(box_holder, request->box_name) == 0 &&
        remove_failed == false) {
        // Box was removed successfully
        response_proto_t *res
            __attribute__((cleanup(response_proto_t_cleanup))) =
                response_proto(0, "\0");
        send_proto_string(wx, REMOVE_BOX_RESPONSE, res);
    } else {
        WARN("Box removal failed!");
        // Error while removing box
        char *error_msg __attribute__((cleanup(str_cleanup))) =
            calloc(MSG_SIZE, sizeof(char));
        snprintf(error_msg, MSG_SIZE, "Failed to remove box with name: %s",
                 request->box_name);

        // Removed attribute cleanup
        response_proto_t *res = response_proto(-1, error_msg);

        send_proto_string(wx, REMOVE_BOX_RESPONSE, res);
    }
    close(wx);
    return;
}

void list_boxes(void *protocol, box_holder_t *box_holder) {
    list_boxes_request_proto_t *request =
        (list_boxes_request_proto_t *)protocol;

    int wr = open_pipe(request->client_named_pipe_path, O_WRONLY);

    pthread_mutex_lock(&box_holder->lock);

    // Send a response for each box
    for (size_t i = 0; i < box_holder->current_size; i++) {
        // Send each request
        box_metadata_t *box = box_holder->boxes[i];
        uint8_t last = 0;
        if (i == box_holder->current_size - 1) {
            last = 1;
        }

        // Creates the response protocol
        pthread_mutex_lock(&box->total_message_size_lock);
        pthread_mutex_lock(&box->has_publisher_lock);
        pthread_mutex_lock(&box->subscribers_count_lock);
        list_boxes_response_proto_t *res = list_boxes_response_proto(
            last, box->name, box->total_message_size, box->has_publisher,
            box->subscribers_count);
        pthread_mutex_unlock(&box->total_message_size_lock);
        pthread_mutex_unlock(&box->has_publisher_lock);
        pthread_mutex_unlock(&box->subscribers_count_lock);

        // Sends the protocol
        send_proto_string(wr, LIST_BOXES_RESPONSE, res);
    }

    // No boxes
    if (box_holder->current_size == 0) {
        char *msg __attribute__((cleanup(str_cleanup))) =
            calloc(BOX_NAME_SIZE, sizeof(char)); // String with 32 '\0's
        list_boxes_response_proto_t *res =
            list_boxes_response_proto(1, msg, 0, false, 0);
        send_proto_string(wr, LIST_BOXES_RESPONSE, res);
    }

    pthread_mutex_unlock(&box_holder->lock);

    close(wr);
}
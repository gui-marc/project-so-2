#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "betterassert.h"
#include "fs/operations.h"
#include "logging.h"
#include "producer-consumer.h"
#include "protocols.h"
#include "requests.h"

void *finish_reading(int fd, uint8_t request_code) {
    void *buffer = NULL;
    ssize_t res = -1;
    switch (request_code) {
    case REGISTER_PUBLISHER:
    case REGISTER_SUBSCRIBER:
    case CREATE_BOX_REQUEST:
    case REMOVE_BOX_REQUEST:
        buffer = malloc(sizeof(request_protocol_t) - sizeof(protocol_t));
        res = read(fd, buffer, sizeof(request_protocol_t) - sizeof(protocol_t));
        break;
    case LIST_BOXES_REQUEST:
        buffer =
            malloc(sizeof(list_boxes_request_protocol_t) - sizeof(protocol_t));
        res = read(fd, buffer,
                   sizeof(list_boxes_request_protocol_t) - sizeof(protocol_t));
        break;
    default:
        WARN("invalid protocol code\n");
        break;
    }
    ALWAYS_ASSERT(res != -1, "Failed reading buffer");
    return buffer;
}

int main(int argc, char **argv) {
    // Must have at least 3 arguments
    if (argc < 3) {
        PANIC("usage: mbroker <register_pipe_name> <max_sessions>");
    }

    const char *register_pipe_name = argv[1];
    const char *max_sessions_str = argv[2];
    const size_t max_sessions = (size_t)atoi(max_sessions_str);

    // producer-consumer queue
    pc_queue_t pc_queue;
    // Creates the producer-consumer queue
    if (pcq_create(&pc_queue, max_sessions) == -1) {
        PANIC("failed to create queue\n");
    }

    // Remove the pipe if it does not exist
    if (unlink(register_pipe_name) != 0 && errno != ENOENT) {
        PANIC("failed to unlink fifo: %s\n", register_pipe_name);
    }

    // Create named pipe (fifo)
    if (mkfifo(register_pipe_name, MKFIFO_PERMS) != 0) {
        PANIC("mkfifo failed: %s\n", register_pipe_name);
    }

    // Create threads
    pthread_t threads[max_sessions];
    for (int i = 0; i < max_sessions; i++) {
        pthread_create(&threads[i], NULL, listen_for_requests, &pc_queue);
    }

    // This waits to other process to write in the pipe
    int rx = open(register_pipe_name, O_RDONLY);
    if (rx == -1) {
        PANIC("failed to open register pipe: %s\n", register_pipe_name);
        remove(register_pipe_name);
    }

    // Listen to events in the register pipe
    while (true) {
        uint8_t prot_code;
        ssize_t ret = read(rx, &prot_code, sizeof(uint8_t));

        printf("%d\n", protocol->base.code);

        void *remaining = finish_reading(rx, protocol->base.code);

        request_protocol_t *req = (request_protocol_t *)remaining;
        printf("%s\n", req->box_name);

        if (ret == 0) {
            INFO("pipe closed\n");
            break; // Stop listening
        } else if (ret == -1) {
            PANIC("failed to read named pipe: %s\n", register_pipe_name);
        }

        void *protocol = malloc(proto_size(prot_code));
        ret = read(rx, protocol, proto_size(prot_code));

        queue_obj_t *obj = malloc(sizeof(queue_obj_t));

        obj->opcode = prot_code;
        obj->protocol = protocol;

        pcq_enqueue(&pc_queue, obj);
    }

    // Wait for all threads to finish
    for (int i = 0; i < max_sessions; i++) {
        pthread_join(threads[i], NULL);
    }

    // Closes the register pipe
    close(rx);

    // Removes the pipe
    if (remove(register_pipe_name) != 0) {
        PANIC("failed to remove named pipe: %s\n", register_pipe_name);
    }

    // Destroys the queue
    pcq_destroy(&pc_queue);
}

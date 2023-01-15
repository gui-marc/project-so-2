#include "betterassert.h"
#include "fs/operations.h"
#include "logging.h"
#include "producer-consumer.h"
#include "requests.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "betterassert.h"
#include "box_metadata.h"
#include "fs/operations.h"
#include "logging.h"
#include "mbroker.h"
#include "producer-consumer.h"
#include "protocols.h"
#include "requests.h"

#define MAX_BOXES 1024

uint8_t sigint_called = 0;

void sigint_handler() {
    DEBUG("Caught SIGINT! Exiting.");

    exit(0);
}

int main(int argc, char **argv) {
    set_log_level(LOG_VERBOSE); // TODO: Remove
    // Must have at least 3 arguments
    if (argc < 3) {
        PANIC("usage: mbroker <register_pipe_name> <max_sessions>");
    }

    box_holder_t box_holder;

    const char *register_pipe_name = argv[1];
    const char *max_sessions_str = argv[2];
    max_sessions = (size_t)atoi(max_sessions_str);
    DEBUG("Creating box holder...")
    if (box_holder_create(&box_holder, MAX_BOXES) == -1) {
        PANIC("Failed to create box holder\n");
    }
    DEBUG("Done creating box holder.")

    // Bootstrap tfs file system
    ALWAYS_ASSERT(tfs_init(NULL) != -1, "Failed to initialize TFS");

    // Redefine SIGINT treatment
    signal(SIGINT, sigint_handler);

    // producer-consumer queue
    pc_queue_t pc_queue;
    // Creates the producer-consumer queue
    if (pcq_create(&pc_queue, max_sessions) == -1) {
        PANIC("failed to create queue\n");
    }

    // create_pipe unlinks existing pipe and does asserts.
    create_pipe(register_pipe_name);

    // Create threads
    pthread_t threads[max_sessions];
    void **args = calloc(2, sizeof(char *));
    args[0] = &pc_queue;
    args[1] = &box_holder;
    for (int i = 0; i < max_sessions; i++) {
        DEBUG("Creating thread %d", i);
        pthread_create(&threads[i], NULL, listen_for_requests, args);
    }
    // This waits to other process to write in the pipe
    DEBUG("Opening register pipe");
    int rx = open(register_pipe_name, O_RDONLY);
    if (rx == -1) {
        if (errno != EINTR) {
            PANIC("failed to open register pipe: %s\n", register_pipe_name);
            remove(register_pipe_name);
        }
    }
    DEBUG("Finshed opening register pipe");

    // Listen to events in the register
    while (sigint_called == 0) {
        uint8_t prot_code = 0;
        DEBUG("Going to read from register pipe, may fall asleep.");
        ssize_t ret = read(rx, &prot_code, sizeof(uint8_t));
        // A dummy writer, to avoid active wait - we never actually use it.
        const int dummy_fd = open(register_pipe_name, O_WRONLY);
        DEBUG("dummy_fd = %d", dummy_fd);
        DEBUG("Read proto code %u", prot_code);

        if (ret == 0) {
            INFO("pipe closed\n");
            continue;
        } else if (ret == -1) {
            PANIC("failed to read named pipe: %s\n", register_pipe_name);
        }

        void *protocol = parse_protocol(rx, prot_code);

        queue_obj_t *obj = calloc(1, sizeof(queue_obj_t));

        obj->opcode = prot_code;
        obj->protocol = protocol;

        DEBUG("enqueue request of protocol: %u", obj->opcode);

        pcq_enqueue(&pc_queue, obj);
    }
}

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
#include "producer-consumer.h"
#include "protocols.h"
#include "requests.h"

#define MAX_BOXES 1024

// Variable used to stop listening for requests
uint8_t must_exit = 0;

/**
 * @brief Set the app to end
 *
 */
void sig_handler() { must_exit = 1; }

int main(int argc, char **argv) {
    // Must have at least 3 arguments
    if (argc < 3) {
        PANIC("usage: mbroker <register_pipe_name> <max_sessions>");
    }

    box_holder_t box_holder;

    const char *register_pipe_name = argv[1];
    const char *max_sessions_str = argv[2];
    size_t max_sessions = (size_t)atoi(max_sessions_str);

    // Creates box holder
    if (box_holder_create(&box_holder, MAX_BOXES) == -1) {
        PANIC("Failed to create box holder\n");
    }

    // Bootstrap tfs file system
    ALWAYS_ASSERT(tfs_init(NULL) != -1, "Failed to initialize TFS");

    // Redefine signals treatment
    signal(SIGINT, sig_handler);
    signal(SIGPIPE, sig_handler);
    signal(SIGQUIT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGUSR1, sig_handler);

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
        pthread_create(&threads[i], NULL, listen_for_requests, args);
    }

    // This waits to other process to write in the pipe
    int rx = open(register_pipe_name, O_RDONLY);
    if (rx == -1) {
        if (errno != EINTR) {
            PANIC("failed to open register pipe: %s\n", register_pipe_name);
            remove(register_pipe_name);
        }
    }

    // Listen to events in the register
    while (must_exit == 0) {
        uint8_t prot_code = 0;
        ssize_t ret = read(rx, &prot_code, sizeof(uint8_t));

        // A dummy writer, to avoid active wait - we never actually use it.
        const int dummy_fd = open(register_pipe_name, O_WRONLY);
        (void)dummy_fd;

        if (ret == 0) {
            INFO("pipe closed\n");
            continue;
        } else if (ret == -1) {
            WARN("Failed to read named pipe: %s", register_pipe_name);
            break;
        }

        void *protocol = parse_protocol(rx, prot_code);

        queue_obj_t *obj = calloc(1, sizeof(queue_obj_t));

        obj->opcode = prot_code;
        obj->protocol = protocol;

        pcq_enqueue(&pc_queue, obj);
    }

    // Sends a signal to end each thread
    for (size_t i = 0; i < max_sessions; i++) {
        pthread_kill(threads[i], SIGUSR1);
    }

    // Waits for each thread to end
    for (size_t i = 0; i < max_sessions; i++) {
        pthread_join(threads[i], NULL);
    }

    pcq_destroy(&pc_queue);
    box_holder_destroy(&box_holder);
    exit(EXIT_SUCCESS);
}

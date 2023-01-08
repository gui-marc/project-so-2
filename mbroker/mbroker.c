#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "fs/operations.h"
#include "logging.h"
#include "producer-consumer.h"
#include "requests.h"

#define MAX_PROTOCOL_SIZE 2048

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
    if (mkfifo(register_pipe_name, 0640) != 0) {
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
    }

    // Listen to events in the register pipe
    while (true) {
        char buffer[MAX_PROTOCOL_SIZE];
        ssize_t ret = read(rx, buffer, MAX_PROTOCOL_SIZE);
        if (ret == 0) {
            INFO("pipe closed\n");
            break; // Stop listening
        } else if (ret == -1) {
            PANIC("failed to read named pipe: %s\n", register_pipe_name);
        }

        pcq_enqueue(&pc_queue, buffer);
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

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "betterassert.h"
#include "logging.h"
#include "protocols.h"
#include "utils.h"

// Tells if the app must continue or not
int sigint_called = 0;

/**
 * @brief Handles the closing of the subscriber
 */
void sigint_handler() { sigint_called = 1; }

int main(int argc, char **argv) {
    set_log_level(LOG_QUIET);
    if (argc != 4) {
        fprintf(stderr,
                "usage: sub <register_pipe_name> <pipe_name> <box_name>\n");
    }

    int messages_received = 0;

    char *register_pipe_name = argv[1]; // Pipe to send the request
    char *pipe_name = argv[2];          // Pipe to receive the responses
    char *box_name = argv[3];

    ALWAYS_ASSERT(argc == 4, "Invalid usage");

    // Creates the pipe to receive responses
    create_pipe(pipe_name);

    // Redefine SIGINT treatment (ignore)
    signal(SIGINT, sigint_handler);

    request_proto_t *request __attribute__((cleanup(request_proto_t_cleanup))) =
        (request_proto_t *)request_proto(pipe_name, box_name);

    ALWAYS_ASSERT(strcmp(pipe_name, request->client_named_pipe_path) == 0,
                  "error while reading name");

    ALWAYS_ASSERT(strcmp(box_name, request->box_name) == 0,
                  "error while reading box_name");

    DEBUG("client_named_pipe: %s\n", request->client_named_pipe_path);

    int wx = open_pipe(register_pipe_name, O_WRONLY);

    send_proto_string(wx, REGISTER_SUBSCRIBER, request);

    int rx = open_pipe(pipe_name, O_RDONLY);
    ALWAYS_ASSERT(rx != -1, "Failed to open pipe %s", pipe_name);

    // Waits until the
    while (!sigint_called) {
        uint8_t opcode = recv_opcode(rx);
        // If it was actually a message
        if (opcode == SUBSCRIBER_MESSAGE) {
            messages_received++;
            basic_msg_proto_t *msg = parse_protocol(rx, opcode);
            fprintf(stdout, "%s\n", msg->msg);
        } else {
            WARN("Didn't expected, ending");
            break;
        }
    }

    fprintf(stdout, "\n%d\n", messages_received);

    gg_close(rx);
    return 0;
}

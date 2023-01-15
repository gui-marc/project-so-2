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

int sigint_called = 0;

void sigint_handler() {
    DEBUG("Caught SIGINT!");
    sigint_called = 1;
}

int main(int argc, char **argv) {
    set_log_level(LOG_VERBOSE);
    if (argc != 4) {
        fprintf(stderr,
                "usage: sub <register_pipe_name> <pipe_name> <box_name>\n");
    }

    char *register_pipe_name = argv[1];
    char *pipe_name = argv[2];
    char *box_name = argv[3];

    ALWAYS_ASSERT(argc == 4, "Invalid usage");

    create_pipe(pipe_name);

    // Redefine SIGINT treatment
    signal(SIGINT, sigint_handler);

    request_proto_t *request =
        (request_proto_t *)request_proto(pipe_name, box_name);

    ALWAYS_ASSERT(strcmp(pipe_name, request->client_named_pipe_path) == 0,
                  "error while reading name");

    ALWAYS_ASSERT(strcmp(box_name, request->box_name) == 0,
                  "error while reading box_name");

    DEBUG("client_named_pipe: %s\n", request->client_named_pipe_path);

    int wx = open(register_pipe_name, O_WRONLY);
    ALWAYS_ASSERT(wx != -1, "Failed to open fifo");

    send_proto_string(wx, REGISTER_SUBSCRIBER, request);

    int rx = open(pipe_name, O_RDONLY);
    ALWAYS_ASSERT(rx != -1, "Failed to open pipe %s", pipe_name);

    // Waits until the
    while (!sigint_called) {
        DEBUG("Receiving messages...");
        uint8_t opcode = recv_opcode(rx);
        DEBUG("Received protocol %d", opcode);
        // If it was actually a message
        if (opcode != 0) {
            basic_msg_proto_t *msg = parse_protocol(rx, opcode);
            fprintf(stdout, "%s\n", msg->msg);
            DEBUG("Received message %s", msg->msg);
        }
    }
    // fprintf(stdout, "")  //FIXME: count nº of messages and print them here
    return 0;
}

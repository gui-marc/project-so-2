#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "betterassert.h"
#include "logging.h"
#include "protocols.h"

int main(int argc, char **argv) {
    set_log_level(LOG_VERBOSE);
    if (argc != 4) {
        fprintf(stderr,
                "usage: pub <register_pipe_name> <pipe_name> <box_name>\n");
        return -1;
    }

    char *register_pipe_name = argv[1];
    char *pipe_name = argv[2];
    char *box_name = argv[3];

    request_proto_t *request =
        (request_proto_t *)request_proto(pipe_name, box_name);

    ALWAYS_ASSERT(strcmp(pipe_name, request->client_named_pipe_path) == 0,
                  "error while reading name");

    ALWAYS_ASSERT(strcmp(box_name, request->box_name) == 0,
                  "error while reading box_name");

    DEBUG("client_named_pipe: %s\n", request->client_named_pipe_path);

    DEBUG("Creating client pipe...");
    create_pipe(pipe_name);

    DEBUG("Opening register pipe.");
    int regpipe_fd = open(register_pipe_name, O_WRONLY);
    ALWAYS_ASSERT(regpipe_fd != -1, "Failed to open register FIFO!");

    DEBUG("Sent register publisher code %u", REGISTER_PUBLISHER);
    send_proto_string(regpipe_fd, REGISTER_PUBLISHER, request);

    DEBUG("Opening client pipe...");
    int pipe_fd = open(pipe_name, O_WRONLY);
    ALWAYS_ASSERT(pipe_fd != -1, "Failed to open client FIFO!");
    int dummy_fd = open(pipe_name, O_RDONLY);

    char *proto_msg;
    DEBUG("Going to read input from stdin.");
    bool to_continue = true;
    char *msg_buf = calloc(MSG_SIZE, sizeof(char));
    while (to_continue) {
        if (fgets(msg_buf, MSG_SIZE, stdin) == NULL) {
            PANIC("fgets failed to read from stdin");
        }
        char *ptr = strchr(msg_buf, '\n');
        if (ptr) {
            *ptr = '\0';
        }
        DEBUG("Going to send string '%s'", msg_buf);

        proto_msg = message_proto(msg_buf);
        send_proto_string(pipe_fd, PUBLISHER_MESSAGE, proto_msg);
        // Guarantee a clean buffer on each iteration
        memset(msg_buf, '\0', MSG_SIZE);
    }

    close(dummy_fd);
    close(pipe_fd);
    return 0;
}

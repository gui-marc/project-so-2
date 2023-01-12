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
    if (argc != 4) {
        fprintf(stderr,
                "usage: pub <register_pipe_name> <pipe_name> <box_name>\n");
    }

    char *register_pipe_name = argv[1];
    char *pipe_name = argv[2];
    char *box_name = argv[3];

    ALWAYS_ASSERT(argc == 4, "Invalid usage");

    request_protocol_t *request =
        (request_protocol_t *)register_publisher_protocol(pipe_name, box_name);

    ALWAYS_ASSERT(strcmp(pipe_name, request->client_named_pipe_path) == 0,
                  "error while reading name");

    ALWAYS_ASSERT(strcmp(box_name, request->box_name) == 0,
                  "error while reading box_name");

    DEBUG("client_named_pipe: %s\n", request->client_named_pipe_path);

    int wx = open(register_pipe_name, O_WRONLY);
    ALWAYS_ASSERT(wx != -1, "Failed to open fifo");

    ssize_t res = write(wx, request, sizeof(request_protocol_t));
    ALWAYS_ASSERT(res == sizeof(request_protocol_t),
                  "error while writing protocol");

    return 0;
}

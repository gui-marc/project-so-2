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
    if (argc != 3) {
        fprintf(stderr,
                "usage: sub <register_pipe_name> <pipe_name> <box_name>\n");
    }

    ALWAYS_ASSERT(argc == 3, "Invalid usage");

    request_protocol_t *request =
        (request_protocol_t *)register_subscriber_protocol(argv[1], argv[2]);

    ALWAYS_ASSERT(strcmp(argv[1], request->client_named_pipe_path) == 0,
                  "error while reading name");

    ALWAYS_ASSERT(strcmp(argv[2], request->box_name) == 0,
                  "error while reading box_name");

    printf("client_named_pipe: %s\n", request->client_named_pipe_path);

    int wx = open(request->client_named_pipe_path, O_WRONLY);
    ALWAYS_ASSERT(wx != -1, "Failed to open fifo");

    ssize_t res = write(wx, request, sizeof(request_protocol_t));
    ALWAYS_ASSERT(res == sizeof(request_protocol_t),
                  "error while writing protocol");

    return 0;
}

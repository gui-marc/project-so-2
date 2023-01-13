#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "betterassert.h"
#include "logging.h"
#include "operations.h"
#include "protocols.h"
#include "string.h"

#define STR_MATCH(str1, str2) (strcmp(str1, str2) == 0)

static int cmp_response(const void *p1, const void *p2) {
    list_boxes_response_proto_t *r1 = (list_boxes_response_proto_t *)p1;
    list_boxes_response_proto_t *r2 = (list_boxes_response_proto_t *)p2;
    return strcmp(r1->box_name, r2->box_name);
}

static void print_usage() {
    fprintf(stderr,
            "usage: \n"
            "   manager <register_pipe_name> <pipe_name> create <box_name>\n"
            "   manager <register_pipe_name> <pipe_name> remove <box_name>\n"
            "   manager <register_pipe_name> <pipe_name> list\n");
}

int list_boxes(const char *server_pipe_name, const char *client_pipe_name) {
    // Sends the request to the mbroker
    list_boxes_request_proto_t *request =
        list_boxes_request_proto(client_pipe_name);
    int wx = open_pipe(server_pipe_name, O_WRONLY);
    send_proto_string(wx, LIST_BOXES_REQUEST, request);

    size_t curr_index = 0;
    // Array for storing responses
    const list_boxes_response_proto_t **responses =
        calloc(tfs_default_params().max_inode_count,
               sizeof(list_boxes_response_proto_t));

    // Reads all messages
    int rx = open_pipe(client_pipe_name, O_RDONLY | O_CREAT);

    // Breaks in the last box or if there was an error in the server side
    while (1) {
        uint8_t opcode = 0;
        ssize_t rs = read(rx, &opcode, sizeof(uint8_t));

        if (opcode == EOF) {
            WARN("Error in the server side");
            break;
        }

        ALWAYS_ASSERT(opcode == LIST_BOXES_RESPONSE, "Received invalid opcode");
        ALWAYS_ASSERT(rs == sizeof(uint8_t), "Failed to read op code");
        list_boxes_response_proto_t *response =
            (list_boxes_response_proto_t *)parse_protocol(rx, opcode);

        responses[curr_index] = response;
        curr_index++;

        if (response->last) {
            break;
        }
    }

    // Sorts all things
    qsort(responses, curr_index, sizeof(list_boxes_response_proto_t),
          cmp_response);

    // Prints all boxes
    for (size_t i = 0; i < curr_index; i++) {
        list_boxes_response_proto_t *res = responses[i];
        fprintf(stdout, "%s %zu %zu %zu\n", res->box_name, res->box_size,
                res->n_publishers, res->n_subscribers);
    }

    // Removes the pipe
    ALWAYS_ASSERT(remove(client_pipe_name) == 0, "Failed to remove pipe");

    // Ok
    return 0;
}

int create_box(const char *server_pipe_name, const char *client_pipe_name,
               const char *box_name) {
    // Sends the request to the mbroker
    request_proto_t *request = request_proto(client_pipe_name, box_name);
    int wx = open_pipe(server_pipe_name, O_WRONLY);
    send_proto_string(wx, CREATE_BOX_REQUEST, request);

    // Waits for the mbroker to send a response
    int rx = open_pipe(client_pipe_name, O_RDONLY | O_CREAT);
    uint8_t opcode = 0;
    ssize_t rs = read(rx, &opcode, sizeof(uint8_t));
    ALWAYS_ASSERT(rs == sizeof(uint8_t), "Invalid read size");
    response_proto_t *response = (response_proto_t *)parse_protocol(rx, opcode);
    ALWAYS_ASSERT(remove(client_pipe_name) == 0, "Failed to remove pipe");

    // If there was an error in the mbroker, print it
    if (response->return_code != 0) {
        fprintf(stderr, "%s", response->error_msg);
        return -1;
    }

    // Ok
    return 0;
}

int remove_box(const char *server_pipe_name, const char *client_pipe_name,
               const char *box_name) {
    // Send the request to the mbroker
    request_proto_t *request = request_proto(client_pipe_name, box_name);
    int wx = open_pipe(server_pipe_name, O_WRONLY);
    send_proto_string(wx, REMOVE_BOX_REQUEST, request);

    // Waits for a response
    int rx = open_pipe(client_pipe_name, O_RDONLY | O_CREAT);
    uint8_t opcode = 0;
    ssize_t rs = read(rx, &opcode, sizeof(uint8_t));
    ALWAYS_ASSERT(rs == sizeof(uint8_t), "Invalid read size");
    response_proto_t *response = (response_proto_t *)parse_protocol(rx, opcode);
    ALWAYS_ASSERT(remove(client_pipe_name) == 0, "Failed to remove pipe");

    // If there was an error in the mbroker side
    if (response->return_code != 0) {
        fprintf(stderr, "%s", response->error_msg);
        return -1;
    }

    return 0;
}

int main(int argc, char **argv) {
    set_log_level(LOG_VERBOSE); // TODO: Remove
    DEBUG("argc = '%d'", argc);
    if (!(argc == 4 || argc == 5)) {
        print_usage();
        return -1;
    }
    char *register_pipe_name = argv[1];

    char *pipe_name = argv[2];

    DEBUG("Register pipe name = '%s'", register_pipe_name);
    char *operation = argv[3];
    DEBUG("Operation = '%s'", operation);

    if (STR_MATCH(operation, "create") && argc == 5) {
        char *box_name = argv[4];
        create_box(register_pipe_name, pipe_name, box_name);
    }
    // We could allow more arguments and just ignore them,
    // but it's better to inform the user of the correct CLI usage
    else if (STR_MATCH(operation, "remove") && argc == 5) {
        char *box_name = argv[4];
        remove_box(register_pipe_name, pipe_name, box_name);
    } else if (STR_MATCH(operation, "list") && argc == 4) {
        list_boxes(register_pipe_name, pipe_name);
    } else {
        print_usage();
        return -1;
    }
    return 0;
}

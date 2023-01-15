#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "betterassert.h"
#include "fs/operations.h"
#include "logging.h"
#include "protocols.h"
#include "string.h"
#include "utils.h"

#define MAX_BOX_NAMES 1024

#define NOX_BOXES_FOUND_ERROR "NO BOXES FOUND\n"

#define STR_MATCH(str1, str2) (strcmp(str1, str2) == 0)

void bubble_sort_responses(list_boxes_response_proto_t **responses,
                           size_t size) {
    size_t i, j;
    for (i = 0; i < size - 1; i++)

        // Last i elements are already in place
        for (j = 0; j < size - i - 1; j++)
            if (strcmp(responses[j]->box_name, responses[j + 1]->box_name) >
                0) {
                // Swap elements
                list_boxes_response_proto_t *tmp = responses[j + 1];
                responses[j + 1] = responses[j];
                responses[j] = tmp;
            }
};

static void print_usage() {
    fprintf(stderr,
            "usage: \n"
            "   manager <register_pipe_name> <pipe_name> create <box_name>\n"
            "   manager <register_pipe_name> <pipe_name> remove <box_name>\n"
            "   manager <register_pipe_name> <pipe_name> list\n");
}

int list_boxes(const char *server_pipe_name, const char *client_pipe_name) {
    DEBUG("start list_boxes");
    // Sends the request to the mbroker
    list_boxes_request_proto_t *request =
        list_boxes_request_proto(client_pipe_name);

    create_pipe(client_pipe_name);

    int wx = open_pipe(server_pipe_name, O_WRONLY);
    send_proto_string(wx, LIST_BOXES_REQUEST, request);

    size_t curr_index = 0;
    // Array for storing responses
    list_boxes_response_proto_t **responses =
        calloc(MAX_BOX_NAMES, sizeof(list_boxes_response_proto_t));

    // Reads all messages
    int rx = open_pipe(client_pipe_name, O_RDONLY);

    // Breaks in the last box or if there was an error in the server side
    while (true) {
        // temporary int opcode
        int t_opcode = 0;
        ssize_t rs = gg_read(rx, &t_opcode, sizeof(uint8_t));

        if (t_opcode == EOF) {
            WARN("Error in the server side");
            break;
        }

        uint8_t opcode = (uint8_t)t_opcode;

        ALWAYS_ASSERT(opcode == LIST_BOXES_RESPONSE, "Received invalid opcode");
        ALWAYS_ASSERT(rs == sizeof(uint8_t), "Failed to read op code");
        list_boxes_response_proto_t *response
            __attribute__((cleanup(ls_boxes_resp_proto_cleanup))) =
                (list_boxes_response_proto_t *)parse_protocol(rx, opcode);

        DEBUG("Received box %s", response->box_name);

        responses[curr_index] = response;
        curr_index++;

        if (response->last) {
            break;
        }
    }

    // Sorts all things
    DEBUG("Sorting boxes");
    bubble_sort_responses(responses, curr_index);

    // Prints all boxes
    DEBUG("Printing boxes");
    DEBUG("current index finalized at %ld", curr_index);
    if (curr_index == 1 && strlen(responses[0]->box_name) == 0) {
        fprintf(stdout, "NO BOXES FOUND\n");
    } else {
        for (size_t i = 0; i < curr_index; i++) {
            DEBUG("Printing box at '%lu'", i);
            list_boxes_response_proto_t *res = responses[i];
            fprintf(stdout, "%s %zu %zu %zu\n", res->box_name, res->box_size,
                    res->n_publishers, res->n_subscribers);
        }
    }
    // Removes the pipe
    ALWAYS_ASSERT(unlink(client_pipe_name) == 0, "Failed to remove pipe");

    return 0;
}

int create_box(const char *server_pipe_name, const char *client_pipe_name,
               const char *box_name) {
    DEBUG("start create_box...");
    // Send the request to the mbroker
    DEBUG("box_name = '%s'", box_name);
    request_proto_t *request __attribute__((cleanup(request_proto_t_cleanup))) =
        request_proto(client_pipe_name, box_name);

    create_pipe(client_pipe_name);
    int wx = open_pipe(server_pipe_name, O_WRONLY);
    send_proto_string(wx, CREATE_BOX_REQUEST, request);
    int rx = open_pipe(client_pipe_name, O_RDONLY);

    uint8_t opcode = 0;
    DEBUG("Waiting for mbroker's response");
    ssize_t rs = gg_read(rx, &opcode, sizeof(uint8_t));
    ALWAYS_ASSERT(rs != -1, "Failed to read op code");

    response_proto_t *response
        __attribute__((cleanup(response_proto_t_cleanup))) =
            (response_proto_t *)parse_protocol(rx, opcode);
    ALWAYS_ASSERT(unlink(client_pipe_name) == 0, "Failed to remove pipe");

    // If there was an error in the mbroker, print it
    if (response->return_code != 0) {
        fprintf(stdout, "ERROR %s\n", response->error_msg);
        return -1;
    } else {
        fprintf(stdout, "OK\n");
        return 0;
    }
}

int remove_box(const char *server_pipe_name, const char *client_pipe_name,
               const char *box_name) {
    DEBUG("start remove_box...");
    // Send the request to the mbroker
    request_proto_t *request = request_proto(client_pipe_name, box_name);

    create_pipe(client_pipe_name);
    int wx = open_pipe(server_pipe_name, O_WRONLY);
    send_proto_string(wx, REMOVE_BOX_REQUEST, request);
    int rx = open_pipe(client_pipe_name, O_RDONLY);

    uint8_t opcode = 0;
    ssize_t rs = gg_read(rx, &opcode, sizeof(uint8_t));
    ALWAYS_ASSERT(rs != -1, "Invalid read size");
    response_proto_t *response
        __attribute__((cleanup(response_proto_t_cleanup))) =
            (response_proto_t *)parse_protocol(rx, opcode);
    ALWAYS_ASSERT(unlink(client_pipe_name) == 0, "Failed to remove pipe");

    // If there was an error in the mbroker side
    if (response->return_code != 0) {
        fprintf(stdout, "ERROR %s\n", response->error_msg);
        return -1;
    } else {
        fprintf(stdout, "OK\n");
        return 0;
    }
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

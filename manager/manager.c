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

#define MAX_BOX_NAMES 1024

#define NOX_BOXES_FOUND_ERROR "NO BOXES FOUND\n"

#define STR_MATCH(str1, str2) (strcmp(str1, str2) == 0)

/**
 * @brief Sorts the list of responses by the box name alphabetically
 *
 * @param responses the list of responses
 * @param size the size of the list
 */
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

/**
 * @brief Prints how to use the manager
 *
 */
static void print_usage() {
    fprintf(stderr,
            "usage: \n"
            "   manager <register_pipe_name> <pipe_name> create <box_name>\n"
            "   manager <register_pipe_name> <pipe_name> remove <box_name>\n"
            "   manager <register_pipe_name> <pipe_name> list\n");
}

/**
 * @brief Method to list boxes. Makes a request to the mbroker then present the
 * boxes sorted.
 *
 * @param server_pipe_name the mbroker named pipe path
 * @param client_pipe_name the client named pipe path
 * @return int 0 if is ok and -1 otherwise
 */
int list_boxes(const char *server_pipe_name, const char *client_pipe_name) {
    // Sends the request to the mbroker
    list_boxes_request_proto_t *request =
        list_boxes_request_proto(client_pipe_name);

    // Creates the pipe to receive the response
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
        ssize_t rs = read(rx, &t_opcode, sizeof(uint8_t));

        if (t_opcode == EOF) {
            WARN("Error in the server side");
            break;
        }

        uint8_t opcode = (uint8_t)t_opcode;

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
    bubble_sort_responses(responses, curr_index);

    // Prints all boxes
    if (curr_index == 1 && strlen(responses[0]->box_name) == 0) {
        fprintf(stdout, "NO BOXES FOUND\n");
    } else {
        for (size_t i = 0; i < curr_index; i++) {
            list_boxes_response_proto_t *res = responses[i];
            fprintf(stdout, "%s %zu %zu %zu\n", res->box_name, res->box_size,
                    res->n_publishers, res->n_subscribers);
        }
    }
    // Removes the pipe
    ALWAYS_ASSERT(unlink(client_pipe_name) == 0, "Failed to remove pipe");

    return 0;
}

/**
 * @brief Handles sending a request for creating a box in the mbroker
 *
 * @param server_pipe_name the mbroker named pipe path
 * @param client_pipe_name the client named pipe path
 * @param box_name the name of the box to be created
 * @return int 0 if was successful and -1 otherwise
 */
int create_box(const char *server_pipe_name, const char *client_pipe_name,
               const char *box_name) {
    // Send the request to the mbroker
    request_proto_t *request = request_proto(client_pipe_name, box_name);

    // Creates the pipe to receive the response
    create_pipe(client_pipe_name);

    // Sends the request to create a box
    int wx = open_pipe(server_pipe_name, O_WRONLY);
    send_proto_string(wx, CREATE_BOX_REQUEST, request);

    // Open the pipe to receive the response
    int rx = open_pipe(client_pipe_name, O_RDONLY);

    uint8_t opcode = 0;
    ssize_t rs = read(rx, &opcode, sizeof(uint8_t));
    ALWAYS_ASSERT(rs != -1, "Failed to read op code");

    response_proto_t *response = (response_proto_t *)parse_protocol(rx, opcode);
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

/**
 * @brief Handles requesting a box removal in the mbroker.
 *
 * @param server_pipe_name the mbroker named pipe path
 * @param client_pipe_name the client named pipe path
 * @param box_name the box to be removed
 * @return int 0 if was successful and -1 otherwise
 */
int remove_box(const char *server_pipe_name, const char *client_pipe_name,
               const char *box_name) {
    // Send the request to the mbroker
    request_proto_t *request = request_proto(client_pipe_name, box_name);

    // Creates the pipe to receive the response
    create_pipe(client_pipe_name);

    // Sends the request to remove a box
    int wx = open_pipe(server_pipe_name, O_WRONLY);
    send_proto_string(wx, REMOVE_BOX_REQUEST, request);

    // Open the pipe to receive the response
    int rx = open_pipe(client_pipe_name, O_RDONLY);

    // The response protocol code
    uint8_t opcode = 0;

    // Waits for the response
    ssize_t rs = read(rx, &opcode, sizeof(uint8_t));
    ALWAYS_ASSERT(rs != -1, "Invalid read size");
    response_proto_t *response = (response_proto_t *)parse_protocol(rx, opcode);
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

/**
 * @brief Parses the right operation in the manager
 *
 * @param argc number of arguments
 * @param argv array containing the arguments
 * @return int
 */
int main(int argc, char **argv) {
    if (!(argc == 4 || argc == 5)) {
        print_usage();
        return -1;
    }

    char *register_pipe_name = argv[1]; // Pipe to send the requests
    char *pipe_name = argv[2];          // Pipe to receive the responses
    char *operation = argv[3];

    if (STR_MATCH(operation, "create") && argc == 5) {
        // Create a box
        char *box_name = argv[4];
        create_box(register_pipe_name, pipe_name, box_name);
    } else if (STR_MATCH(operation, "remove") && argc == 5) {
        // Remove a box
        char *box_name = argv[4];
        remove_box(register_pipe_name, pipe_name, box_name);
    } else if (STR_MATCH(operation, "list") && argc == 4) {
        // List the boxes
        list_boxes(register_pipe_name, pipe_name);
    } else {
        // Invalid operation
        print_usage();
        return -1;
    }
    return 0;
}

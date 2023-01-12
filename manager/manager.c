#include "logging.h"
#include "protocols.h"
#include <string.h>
#include <unistd.h>

#define STR_MATCH(str1, str2) (strcmp(str1, str2) == 0)

enum { CREATE_BOX = 0, REMOVE_BOX, LIST_BOXES };

/*
Assim que é lançado, o manager:

Envia o pedido à mbroker;
Recebe a resposta no named pipe criado pelo próprio manager;
Imprime a resposta e termina.
*/
// print_usage();
// Steps:
// Get register pipe name from stdin, validate it (ensure the FIFO exists,
// attempt to connect to server)

static void print_usage() {
    fprintf(stderr,
            "usage: \n"
            "   manager <register_pipe_name> <pipe_name> create <box_name>\n"
            "   manager <register_pipe_name> <pipe_name> remove <box_name>\n"
            "   manager <register_pipe_name> <pipe_name> list\n");
}

int handle_create(const char *reply) {
    (void)reply;
    return 0;
}

int handle_list() { 
    int last = 0;
    char *reply = alloc_protocol_message();
    while (last == 0) {
        ALWAYS_ASSERT(read(npipe, reply, PROTOCOL_MESSAGE_LENGTH),
                      "Failed to read mbroker reply");
        int code = get_code(reply);
        if (code == CODE_LIST_BOXES_RESPONSE) {
            last = get_last(reply);
            char *box_name = get_box_name(reply);
            uint64_t box_size = get_box_size(reply);
            uint64_t n_publishers = get_n_publishers(reply);
            printf("%s %lu %lu", box_name, box_size, n_publishers);
        }
    }
    return 0;
}

int handle_remove(const char *reply) {
    (void)reply;
    return 0;
}

int send_msg(int op, char **argv) {
    char regpipe_path[NAMED_PIPE_PATH_LIM] = argv[1];
    char npipe_path[NAMED_PIPE_PATH_LIM] =argv[2];
    DEBUG("Register pipe name = '%s'", regpipe_path);
    DEBUG("Named pipe name = '%s'", npipe_path);

    create_pipe(npipe_path);
    int npipe = open_pipe(npipe_path);

    int regpipe = open_pipe(regpipe_path);

    char *reply = alloc_protocol_message();

    switch (op) {
    case CREATE_BOX:
        char boxname[BOX_NAME_LIM] = argv[3];
        char *msg = create_box_request_protocol(npipe_path, boxname);
        ALWAYS_ASSERT(write(regpipe, msg, PROTOCOL_MESSAGE_LENGTH),
                      "Failed to write msg to register pipe");
        ALWAYS_ASSERT(read(npipe, reply, PROTOCOL_MESSAGE_LENGTH),
                      "Failed to read mbroker reply");
        handle_create(reply);

    case REMOVE_BOX:
        char boxname[BOX_NAME_LIM] = argv[3];
        char *msg = remove_box_request_protocol(npipe_path, boxname);
        ALWAYS_ASSERT(write(regpipe, msg, PROTOCOL_MESSAGE_LENGTH),
                      "Failed to write msg to register pipe");
        ALWAYS_ASSERT(read(npipe, reply, PROTOCOL_MESSAGE_LENGTH),
                      "Failed to read mbroker reply");
        handle_remove(reply);

    case LIST_BOXES:
        char *msg = list_boxes_request_protocol(npipe_path);
        ALWAYS_ASSERT(write(regpipe, msg, PROTOCOL_MESSAGE_LENGTH),
                      "Failed to write msg to register pipe");
        handle_list(npipe);
    default:
        return -1;
    };
}

int main(int argc, char **argv) {
    set_log_level(LOG_VERBOSE); // TODO: Remove
    int op = -1;

    DEBUG("argc = '%d'", argc);
    if (!(argc == 3 || argc == 4)) {
        print_usage();
        return -1;
    }
    char *operation = argv[2];
    DEBUG("Raw Operation = '%s'", operation);

    // Todo: ensure correct argc AND create a function to sanitize alphanumeric
    // words
    if (STR_MATCH(operation, "create") && argc == 4) {
        op = CREATE_BOX;
    } else if (STR_MATCH(operation, "remove") && argc == 4) {
        op = REMOVE_BOX;
    } else if (STR_MATCH(operation, "list") && argc == 3) {
        op = LIST_BOXES;
    } else {
        print_usage();
        return -1;
    }

    send_msg(op, argv);
    return 0;
}

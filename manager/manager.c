#include "logging.h"
#include "string.h"

#define STR_MATCH(str1, str2) (strcmp(str1, str2) == 0)

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
    fprintf(stderr, "usage: \n"
                    "   manager <register_pipe_name> create <box_name>\n"
                    "   manager <register_pipe_name> remove <box_name>\n"
                    "   manager <register_pipe_name> list\n");
}

int list_boxes() {
    // Ensure we get the boxes from the server and order them alphabetically
    // before listing, client-side
    return 0;
}

int create_box(const char *boxname) {
    DEBUG("Creating box '%s'", boxname);
    WARN("unimplemented");
    return 0;
}

int remove_box(const char *boxname) {
    DEBUG("Removing box '%s'", boxname);
    WARN("unimplemented");
    return 0;
}

int init_manager() { return 0; }

int main(int argc, char **argv) {

    set_log_level(LOG_VERBOSE); // TODO: Remove
    DEBUG("argc = '%d'", argc);
    if (!(argc == 3 || argc == 4)) {
        print_usage();
        return -1;
    }
    char *register_pipe_name = argv[1];

    DEBUG("Register pipe name = '%s'", register_pipe_name);
    char *operation = argv[2];
    DEBUG("Operation = '%s'", operation);
    // Todo: ensure correct argc AND create a function to sanitize alphanumeric
    // words
    if (STR_MATCH(operation, "create") && argc == 4) {
        char *boxname = argv[2];
        create_box(boxname);
    }
    // We could allow more arguments and just ignore them,
    // but it's better to inform the user of the correct CLI usage
    else if (STR_MATCH(operation, "remove") && argc == 4) {
        char *boxname = argv[2];
        create_box(boxname);
    } else if (STR_MATCH(operation, "list") && argc == 3) {
        list_boxes();
    } else {
        print_usage();
        return -1;
    }
    return 0;
}

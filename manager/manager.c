#include "logging.h"
#include "string.h"

#define STR_MATCH(str1, str2) (strcmp(str1, str2) == 0)

#define DEBUG(str) (fprintf(stderr, "DEBUG: %s\n", str))

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
    fprintf(stderr, "DEBUG: Creating box %s", boxname);
    WARN("unimplemented");
    return 0; 
}

int remove_box(const char *boxname) {
    fprintf(stderr, "Removing box %s", boxname);
    DEBUG(("Removing box %s", boxname));
    (void)boxname;
    WARN("unimplemented");
     return 0;
}

int main(int argc, char **argv) {
    fprintf(stderr, "DEBUG: argc: %d\n", argc);
    if (argc < 2) {
        // fprintf(stderr, "Invalid number of arguments\n");
        print_usage();
        return -1;
    }
    char *register_pipe_name = argv[1];

    fprintf(stderr, "DEBUG: Register pipe name: %s\n", register_pipe_name);
    char *operation = argv[2];
    fprintf(stderr, "DEBUG: Operation '%s'\n", operation);

    if (STR_MATCH(operation, "create") && argc == 3) {
        char *boxname = argv[2];
        create_box(boxname);
    }
    // We could allow more arguments and just ignore them,
    // but it's better to inform the user of the correct CLI usage
    else if (STR_MATCH(operation, "remove") && argc == 3) {
        char *boxname = argv[2];
        create_box(boxname);
    }
     else if (STR_MATCH(operation, "list") && argc == 2) {
        list_boxes();
    } 
    else {
        print_usage();
        return -1;
    }
    return 0;
}

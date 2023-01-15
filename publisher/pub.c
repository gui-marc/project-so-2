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
#include "utils.h"

int main(int argc, char **argv) {
    set_log_level(LOG_NORMAL);
    if (argc != 4) {
        fprintf(stderr,
                "usage: pub <register_pipe_name> <pipe_name> <box_name>\n");
        return -1;
    }

    // Ignores the SIGPIPE
    signal(SIGPIPE, SIG_IGN);

    char *register_pipe_name = argv[1];
    char *pipe_name = argv[2];
    char *box_name = argv[3];

    request_proto_t *request =
        (request_proto_t *)request_proto(pipe_name, box_name);

    ALWAYS_ASSERT(strcmp(pipe_name, request->client_named_pipe_path) == 0,
                  "error while reading name");

    ALWAYS_ASSERT(strcmp(box_name, request->box_name) == 0,
                  "error while reading box_name");

    create_pipe(pipe_name);

    DEBUG("Opening register pipe.");
    int regpipe_fd = open_pipe(register_pipe_name, O_WRONLY, true);

    DEBUG("Sent register publisher code %u", REGISTER_PUBLISHER);
    send_proto_string(regpipe_fd, REGISTER_PUBLISHER, request);
    gg_close(regpipe_fd);
    DEBUG("Opening client pipe...");
    int pipe_fd = open_pipe(pipe_name, O_WRONLY, true);

    char *proto_msg;
    bool to_continue = true;
    char *msg_buf __attribute__((cleanup(str_cleanup))) =
        gg_calloc(MSG_SIZE, sizeof(char));
    int ret = 0;
    DEBUG("Entering publisher loop...");
    while (to_continue) {
        if (fgets(msg_buf, MSG_SIZE, stdin) == NULL) {
            PANIC("fgets failed to read from stdin");
        }
        char *ptr = strchr(msg_buf, '\n');
        if (ptr) {
            *ptr = '\0';
        }

        proto_msg = message_proto(msg_buf);

        ret = send_proto_string(pipe_fd, PUBLISHER_MESSAGE, proto_msg);
        if (ret == -1) {
            INFO("mbroker closed client pipe - quitting.");
            break;
        }
        // Guarantee a clean buffer on each iteration
        memset(msg_buf, '\0', MSG_SIZE);
    }
    // "Se o publisher receber um EOF (End Of File, por exemplo, com um
    // Ctrl-D)," "deve encerrar a sessão fechando o named pipe.""
    DEBUG("Received SIGPIPE (technically EPIPE), quitting")
    gg_close(pipe_fd);
    unlink(pipe_name);
    return 0;
}

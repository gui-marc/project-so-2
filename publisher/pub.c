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
    set_log_level(LOG_VERBOSE);
    if (argc != 4) {
        fprintf(stderr,
                "usage: pub <register_pipe_name> <pipe_name> <box_name>\n");
        return -1;
    }
    // https://piazza.com/class/l92u0ocmbv05rk/post/108
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

    DEBUG("client_named_pipe: %s\n", request->client_named_pipe_path);

    DEBUG("Creating client pipe...");
    create_pipe(pipe_name);

    DEBUG("Opening register pipe.");
    int regpipe_fd = open(register_pipe_name, O_WRONLY);
    ALWAYS_ASSERT(regpipe_fd != -1, "Failed to open register FIFO!");

    DEBUG("Sent register publisher code %u", REGISTER_PUBLISHER);
    send_proto_string(regpipe_fd, REGISTER_PUBLISHER, request);
    ALWAYS_ASSERT(close(regpipe_fd) == 0, "Failed to close register pipe!");

    DEBUG("Opening client pipe...");
    int pipe_fd = open(pipe_name, O_WRONLY);
    ALWAYS_ASSERT(pipe_fd != -1, "Failed to open client FIFO!");

    char *proto_msg;
    DEBUG("Going to read input from stdin.");
    bool to_continue = true;
    char *msg_buf __attribute__((cleanup(str_cleanup))) =
        calloc(MSG_SIZE, sizeof(char));
    int ret = 0;
    while (to_continue) {
        if (fgets(msg_buf, MSG_SIZE, stdin) == NULL) {
            PANIC("fgets failed to read from stdin");
        }
        char *ptr = strchr(msg_buf, '\n');
        if (ptr) {
            *ptr = '\0';
        }
        DEBUG("Going to send string '%s' through client pipe '%s'", msg_buf,
              pipe_name);

        proto_msg = message_proto(msg_buf);

        ret = send_proto_string(pipe_fd, PUBLISHER_MESSAGE, proto_msg);
        DEBUG("ret = %d", ret);
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
    close(pipe_fd);
    return 0;
}

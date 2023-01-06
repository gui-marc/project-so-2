#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "fs/operations.h"
#include "logging.h"

int main(int argc, char **argv) {
    if (argc < 3) {
        PANIC("usage: mbroker <register_pipe_name> <max_sessions>");
    }

    const char *register_pipe_name = argv[1];
    const char *max_sessions_str = argv[2];
    const int max_sessions = atoi(max_sessions_str);
    (void)max_sessions;

    // Remove the pipe if it does not exist
    if (unlink(register_pipe_name) != 0 && errno != ENOENT) {
        PANIC("failed to unlink fifo: %s\n", register_pipe_name);
    }

    // Create named pipe (fifo)
    if (mkfifo(register_pipe_name, 0640) != 0) {
        PANIC("mkfifo failed: %s\n", register_pipe_name);
    }

    // Removes the pipe
    if (remove(register_pipe_name) != 0) {
        PANIC("failed to remove named pipe: %s\n", register_pipe_name);
    }
}

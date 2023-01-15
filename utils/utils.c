#include "betterassert.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
/**
 * Wrappers to functions that result in syscalls, to handle EINTR.
 * Syscalls might fail if a signal gets issued, simply retry them until that
 * doesn't happen.
 */

/*
 * Functions to be used with __atribute__(cleanup(..))
 */

void mem_cleanup(void **ptr) {
    if (*ptr != NULL) {
        free(*ptr);
        *ptr = NULL;
    }
}

void str_cleanup(char **ptr) { mem_cleanup((void **)ptr); }

void ustr_cleanup(unsigned char **ptr) { mem_cleanup((void **)ptr); }

/**
 * Wrappers to functions that may error with EINTR.
 * Syscalls might fail if a signal gets issued, simply retry them until that
 * doesn't happen.
 * Also contains functions to avoid common pitfalls (e.g double free).
 * @return -1 on error, 0 on success.
 */
int gg_open(const char *path, int flag) {
    int r = -1;
    while (true) {
        r = open(path, flag);
        if (r != -1) {
            return r;
        }
        if (errno == EINTR) {
            continue;
        }
    }
    return r;
}

void gg_free(void **ptr) { mem_cleanup(ptr); }

void gg_close(const int fd) {
    int r = -1;
    while (true) {
        r = close(fd);
        if (r != -1) {
            return;
        }
        if (errno == EINTR) {
            continue;
        }
    }
    if (r == -1) {
        WARN("Call to gg_close failed: %s", strerror(errno));
    }
}

ssize_t gg_read(const int fd, void *buf, size_t count) {
    ssize_t r = -1;
    while (true) {
        r = read(fd, buf, count);
        if (r != -1) {
            return r;
        }
        if (errno == EINTR) {
            continue;
        }
    }
    return r;
}

void *gg_calloc(size_t n, size_t size) {
    void *ptr = NULL;
    ptr = calloc(n, size);
    ALWAYS_ASSERT(ptr != NULL, "Call to calloc failed, err ='%s'",
                  strerror(errno));
    return ptr;
}

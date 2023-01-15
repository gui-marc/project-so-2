#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
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

void mutex_cleanup(pthread_mutex_t *mutex) { pthread_mutex_destroy(mutex); }

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "logging.h"
#include "protocols.h"
#include "requests.h"

void *listen_for_requests(void *queue) {
    while (true) {
        char *protocol = (char *)pcq_dequeue((pc_queue_t *)queue);
        u_int8_t code = (u_int8_t)protocol[0] - '0';
        parse_request(code, protocol);
        free(protocol);
    }
    return NULL;
}

void parse_request(u_int8_t request_code, void *protocol) {
    switch (request_code) {
    case CODE_REGISTER_PUBLISHER:
        register_publisher(protocol);
        break;
    case CODE_REGISTER_SUBSCRIBER:
        register_subscriber(protocol);
        break;
    case CODE_CREATE_BOX_REQUEST:
        create_box(protocol);
        break;
    case CODE_REMOVE_BOX_REQUEST:
        remove_box(protocol);
        break;
    case CODE_LIST_BOXES_REQUEST:
        list_boxes(protocol);
        break;
    default:
        WARN("invalid protocol code\n");
        break;
    }
}

void register_publisher(void *protocol) {
    (void)protocol;
    WARN("not implemented\n"); // Todo: implement me
}

void register_subscriber(void *protocol) {
    (void)protocol;
    WARN("not implemented\n"); // Todo: implement me
}

void create_box(void *protocol) {
    (void)protocol;
    WARN("not implemented\n"); // Todo: implement me
}

void remove_box(void *protocol) {
    (void)protocol;
    WARN("not implemented\n"); // Todo: implement me
}

void list_boxes(void *protocol) {
    (void)protocol;
    WARN("not implemented\n"); // Todo: implement me
}
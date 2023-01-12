#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "logging.h"
#include "protocols.h"
#include "requests.h"

void *listen_for_requests(void *queue) {
    while (true) {
        protocol_t *protocol = (protocol_t *)pcq_dequeue((pc_queue_t *)queue);
        uint8_t code = protocol->base.code;
        parse_request(code, protocol);
        free(protocol);
    }
    return NULL;
}

void parse_request(uint8_t request_code, protocol_t *protocol) {
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

void register_publisher(protocol_t *protocol) {
    (void)protocol;
    WARN("not implemented\n"); // Todo: implement me
}

void register_subscriber(protocol_t *protocol) {
    (void)protocol;
    WARN("not implemented\n"); // Todo: implement me
}

void create_box(protocol_t *protocol) {
    (void)protocol;
    WARN("not implemented\n"); // Todo: implement me
}

void remove_box(protocol_t *protocol) {
    (void)protocol;
    WARN("not implemented\n"); // Todo: implement me
}

void list_boxes(protocol_t *protocol) {
    (void)protocol;
    WARN("not implemented\n"); // Todo: implement me
}
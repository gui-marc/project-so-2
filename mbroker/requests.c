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
        queue_obj_t *obj = (queue_obj_t *)pcq_dequeue((pc_queue_t *)queue);
        parse_request(obj);
        free(obj->protocol);
        free(obj);
    }
    return NULL;
}

void parse_request(queue_obj_t *obj) {
    switch (obj->opcode) {
    case REGISTER_PUBLISHER:
        register_publisher(obj->protocol);
        break;
    case REGISTER_SUBSCRIBER:
        register_subscriber(obj->protocol);
        break;
    case CREATE_BOX_REQUEST:
        create_box(obj->protocol);
        break;
    case REMOVE_BOX_REQUEST:
        remove_box(obj->protocol);
        break;
    case LIST_BOXES_REQUEST:
        list_boxes(obj->protocol);
        break;
    default:
        WARN("invalid protocol code\n");
        break;
    }
}

void register_publisher(queue_obj_t *obj) {
    (void)obj;
    WARN("not implemented\n"); // Todo: implement me
}

void register_subscriber(queue_obj_t *obj) {
    (void)obj;
    WARN("not implemented\n"); // Todo: implement me
}

void create_box(queue_obj_t *obj) {
    (void)obj;
    WARN("not implemented\n"); // Todo: implement me
}

void remove_box(queue_obj_t *obj) {
    (void)obj;
    WARN("not implemented\n"); // Todo: implement me
}

void list_boxes(queue_obj_t *obj) {
    (void)obj;
    WARN("not implemented\n"); // Todo: implement me
}
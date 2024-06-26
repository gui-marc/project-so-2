#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "betterassert.h"
#include "logging.h"
#include "protocols.h"

// Reads an opcode from an open named pipe.
//  -1 on fail.
uint8_t recv_opcode(const int fd) {
    uint8_t opcode;
    ALWAYS_ASSERT(fd != -1, "Invalid file descriptor");
    size_t buf_size = sizeof(uint8_t);
    ALWAYS_ASSERT(read(fd, &opcode, buf_size) == buf_size,
                  "Failed to read opcode");
    ALWAYS_ASSERT(opcode != 0, "Failed to convert opcode");
    return opcode;
}

void send_proto_string(const int fd, const uint8_t opcode, const void *proto) {
    ALWAYS_ASSERT(fd != -1, "Invalid file descriptor");
    size_t size = proto_size(opcode) + sizeof(uint8_t);
    unsigned char *final = malloc(size);
    DEBUG("Alloc size %lu", size);
    uint8_t saved_opcode;

    memcpy(final, &opcode, sizeof(uint8_t));
    memcpy(&saved_opcode, final, sizeof(uint8_t));
    ALWAYS_ASSERT(saved_opcode == opcode, "Failed to save opcode");

    memcpy(final + sizeof(uint8_t), proto, size - sizeof(uint8_t));
    memcpy(&saved_opcode, final, sizeof(uint8_t));
    ALWAYS_ASSERT(saved_opcode == opcode, "Failed to save opcode");

    ALWAYS_ASSERT(write(fd, final, size) == size, "Failed to write proto");
    free(final);
}

void create_pipe(const char npipe_path[NPIPE_PATH_SIZE]) {
    ALWAYS_ASSERT(unlink(npipe_path) == 0,
                  "Failed to cleanup/unlink client named pipe.");
    ALWAYS_ASSERT(mkfifo(npipe_path, MKFIFO_PERMS) == 0,
                  "Failed to create client named pipe.");
}

int open_pipe(const char npipe_path[NPIPE_PATH_SIZE], int _flag) {
    int fd = open(npipe_path, _flag);
    ALWAYS_ASSERT(fd != -1, "Failed to open named pipe");
    return fd;
}

void *parse_protocol(const int rx, const uint8_t opcode) {
    DEBUG("parsing protocol %u", opcode);
    size_t proto_sz = proto_size(opcode);
    void *protocol = malloc(proto_sz);
    ssize_t sz = read(rx, protocol, proto_sz);
    ALWAYS_ASSERT(proto_sz == sz, "Failed to read protocol");
    return protocol;
}

void *request_proto(const char *client_named_pipe_path, const char *box_name) {
    request_proto_t *p = malloc(sizeof(request_proto_t));
    strcpy(p->box_name, box_name);
    strcpy(p->client_named_pipe_path, client_named_pipe_path);
    return p;
}

void *response_proto(int32_t return_code, const char *error_message) {
    response_proto_t *p = malloc(sizeof(response_proto_t));
    p->return_code = return_code;
    strcpy(p->error_msg, error_message);
    return p;
}

void *list_boxes_request_proto(const char *client_named_pipe_path) {
    list_boxes_request_proto_t *p =
        calloc(1, sizeof(list_boxes_request_proto_t));
    strcpy(p->client_named_pipe_path, client_named_pipe_path);
    return p;
}

void *list_boxes_response_proto(const uint8_t last, const char *box_name,
                                const uint64_t box_size,
                                const uint64_t n_publishers,
                                const uint64_t n_subscribers) {
    list_boxes_response_proto_t *p =
        malloc(sizeof(list_boxes_response_proto_t));
    p->last = last;
    p->box_size = box_size;
    p->n_publishers = n_publishers;
    p->n_subscribers = n_subscribers;
    strcpy(p->box_name, box_name);
    return p;
}

void *message_proto(const char *message) {
    basic_msg_proto_t *p = malloc(sizeof(basic_msg_proto_t));
    strcpy(p->msg, message);
    return p;
}

size_t proto_size(uint8_t code) {
    size_t sz = 0;
    switch (code) {
    case REGISTER_PUBLISHER:
    case REGISTER_SUBSCRIBER:
    case CREATE_BOX_REQUEST:
    case REMOVE_BOX_REQUEST:
        sz = sizeof(request_proto_t);
        break;
    case LIST_BOXES_REQUEST:
        sz = sizeof(list_boxes_request_proto_t);
        break;
    case REMOVE_BOX_RESPONSE:
    case CREATE_BOX_RESPONSE:
        sz = sizeof(response_proto_t);
        break;
    case LIST_BOXES_RESPONSE:
        sz = sizeof(list_boxes_response_proto_t);
        break;
    default:
        PANIC("invalid proto code\n");
        break;
    }
    return sz;
}
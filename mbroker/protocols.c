#include "protocols.h"
#include "logging.h"

char *register_publisher_protocol(char client_named_pipe_path[256],
                                  char box_name[32]) {
    (void)client_named_pipe_path;
    (void)box_name;
    char *protocol_message = NULL;
    WARN("unimplemented method"); // Todo: implement me
    return protocol_message;
}

char *register_subscriber_protocol(char client_named_pipe_path[256],
                                   char box_name[32]) {
    (void)client_named_pipe_path;
    (void)box_name;
    char *protocol_message = NULL;
    WARN("unimplemented method"); // Todo: implement me
    return protocol_message;
}

char *create_box_request_protocol(char client_named_pipe_path[256],
                                  char box_name[32]) {
    (void)client_named_pipe_path;
    (void)box_name;
    char *protocol_message = NULL;
    WARN("unimplemented method"); // Todo: implement me
    return protocol_message;
}

char *create_box_response_protocol(int32_t return_code,
                                   char error_message[1024]) {
    (void)return_code;
    (void)error_message;
    char *protocol_message = NULL;
    WARN("unimplemented method"); // Todo: implement me
    return protocol_message;
}

char *remove_box_request_protocol(char client_named_pipe_path[256],
                                  char box_name[32]) {
    (void)client_named_pipe_path;
    (void)box_name;
    char *protocol_message = NULL;
    WARN("unimplemented method"); // Todo: implement me
    return protocol_message;
}

char *remove_box_response_protocol(int32_t return_code,
                                   char error_message[1024]) {
    (void)return_code;
    (void)error_message;
    char *protocol_message = NULL;
    WARN("unimplemented method"); // Todo: implement me
    return protocol_message;
}

char *list_boxes_request_protocol(char client_named_pipe_path[256]) {
    (void)client_named_pipe_path;
    char *protocol_message = NULL;
    WARN("unimplemented method"); // Todo: implement me
    return protocol_message;
}

char *list_boxes_response_protocol(uint8_t last, char box_name[32],
                                   uint64_t box_size, uint64_t n_publishers,
                                   uint64_t n_subscribers) {
    (void)last;
    (void)box_name;
    (void)box_size;
    (void)n_publishers;
    (void)n_subscribers;
    char *protocol_message = NULL;
    WARN("unimplemented method"); // Todo: implement me
    return protocol_message;
}

char *publisher_message_protocol(char message[1024]) {
    (void)message;
    char *protocol_message = NULL;
    WARN("unimplemented method"); // Todo: implement me
    return protocol_message;
}

char *subscriber_message_protocol(char message[1024]) {
    (void)message;
    char *protocol_message = NULL;
    WARN("unimplemented method"); // Todo: implement me
    return protocol_message;
}

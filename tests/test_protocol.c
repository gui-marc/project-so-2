#include "../protocol/protocols.h"
#include "../utils/betterassert.h"

#define PIPE_PATH "pipe"
#define BOX_NAME "box"

int main() {
    // Create some protocols
    protocol_t *register_publisher =
        (protocol_t *)register_publisher_protocol(PIPE_PATH, BOX_NAME);
    protocol_t *register_subscriber =
        (protocol_t *)register_subscriber_protocol(PIPE_PATH, BOX_NAME);
    protocol_t *create_box_req =
        (protocol_t *)create_box_request_protocol(PIPE_PATH, BOX_NAME);
    protocol_t *create_box_res =
        (protocol_t *)create_box_response_protocol(0, "");

    ALWAYS_ASSERT(register_publisher != NULL,
                  "register_publisher should not be null");
    ALWAYS_ASSERT(register_subscriber != NULL,
                  "register_subscriber should not be null");
    ALWAYS_ASSERT(create_box_req != NULL, "create_box_req should not be null");
    ALWAYS_ASSERT(create_box_res != NULL,
                  "register_publisher should not be null");

    // All must have the code property
    uint8_t register_publisher_code = register_publisher->base.code;
    ALWAYS_ASSERT(register_publisher_code == REGISTER_PUBLISHER, "code");
    uint8_t register_subscriber_code = register_subscriber->base.code;
    ALWAYS_ASSERT(register_subscriber_code == REGISTER_SUBSCRIBER, "code");
    uint8_t create_box_req_code = create_box_req->base.code;
    ALWAYS_ASSERT(create_box_req_code == CREATE_BOX_REQUEST, "code");
    uint8_t create_box_res_code = create_box_res->base.code;
    ALWAYS_ASSERT(create_box_res_code == CREATE_BOX_RESPONSE, "code");
}
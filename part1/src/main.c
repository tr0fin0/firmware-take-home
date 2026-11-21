#include "payload.h"
#include <stdint.h>

void test_example(void) {
    uint8_t bytes_example[PAYLOAD_SIZE_BYTES] = {
        0x21, 0x51, 0x20, 0xF1, 0x00, 0x10, 0xEF, 0xF1, 0x59, 0x5A, 0x05, 0x65
    };

    payload_t payload_example = {0};
    payload_error_t error = payload_decode(&payload_example, bytes_example);

    if (error == PAYLOAD_NO_ERROR) {
        payload_print(&payload_example);
    } else {
        payload_print_error(error);
    }
}

int main(void) {
    test_example();

    return 0;
}
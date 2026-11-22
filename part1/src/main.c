#include "payload.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const uint8_t bytes_example[PAYLOAD_SIZE_BYTES] = {
    0x21, 0x51, 0x20, 0xF1, 0x00, 0x10, 0xEF, 0xF1, 0x59, 0x5A, 0x05, 0x65
};

static void copy_bytes_example(uint8_t bytes[PAYLOAD_SIZE_BYTES]) {
    memcpy(bytes, bytes_example, PAYLOAD_SIZE_BYTES);
}


void test_decode_encode(void) {
    payload_error_t error = PAYLOAD_NO_ERROR;

    uint8_t bytes_decode[PAYLOAD_SIZE_BYTES];
    copy_bytes_example(bytes_decode);

    payload_t payload_example = {0};
    error = payload_decode(&payload_example, bytes_decode);

    if (error != PAYLOAD_NO_ERROR) {
        payload_print_error(error);
    }

    uint8_t bytes_encode[PAYLOAD_SIZE_BYTES] = {0};
    error = payload_encode(payload_example, bytes_encode);

    if (error != PAYLOAD_NO_ERROR) {
        payload_print_error(error);
    }

    bool transaction_success = true;
    for (uint8_t i = 0; i < PAYLOAD_SIZE_BYTES; i++) {
        if (bytes_decode[i] != bytes_encode[i]) {
            transaction_success = false;
            break;
        }
    }

    if (transaction_success) {
        printf("[success] %s()\n", __func__);
    }
}

int main(void) {
    test_decode_encode();

    return 0;
}
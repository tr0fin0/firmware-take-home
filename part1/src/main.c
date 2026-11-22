#include <assert.h>
#include "payload.h"
#include "payload_constants.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const uint8_t bytes_example[PAYLOAD_SIZE_BYTES] = {
    0x21, 0x51, 0x20, 0xF1, 0x00, 0x10, 0xEF, 0xF1, 0x59, 0x5A, 0x05, 0x65
};

static void copy_bytes_example(uint8_t bytes[PAYLOAD_SIZE_BYTES]) {
    memcpy(bytes, bytes_example, PAYLOAD_SIZE_BYTES);
}

static void decode_assert_error(
    uint8_t bytes[PAYLOAD_SIZE_BYTES],
    payload_error_t expected_error
) {
    payload_t payload = {0};
    payload_error_t error = payload_decode(&payload, bytes);

    assert(error == expected_error);
}

static void set_battery_mv(
    uint8_t bytes[PAYLOAD_SIZE_BYTES],
    payload_battery_mv_t battery_mv
) {
    bytes[5] = (uint8_t) ((bytes[5] & 0x03) | ((battery_mv & 0x003F) << 2));
    bytes[6] = (uint8_t) ((bytes[6] & 0xC0) | ((battery_mv >> 6) & 0x003F));
}

static void update_crc8(uint8_t bytes[PAYLOAD_SIZE_BYTES]) {
    crc8_config_t crc8_config = {
        .init_value     = 0x00,
        .polynomial     = 0x07,
        .reflection_in  = false,
        .reflection_out = false,
        .xor_out        = 0x00
    };

    bytes[PAYLOAD_SIZE_BYTES - 1] = crc8_compute(
        crc8_config, bytes, PAYLOAD_SIZE_BYTES - 1
    );
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

void test_error_decode_battery_above_maximal(void) {
    uint8_t bytes[PAYLOAD_SIZE_BYTES];
    copy_bytes_example(bytes);

    set_battery_mv(
        bytes,
        (payload_battery_mv_t) (PAYLOAD_BATTERY_MAXIMUM_MV + 1U)
    );

    update_crc8(bytes);

    decode_assert_error(bytes, PAYLOAD_ERROR_BATTERY_ABOVE_MAXIMAL);

    printf("[success] %s()\n", __func__);
}

void test_error_decode_crc8(void) {
    uint8_t bytes[PAYLOAD_SIZE_BYTES];
    copy_bytes_example(bytes);

    bytes[PAYLOAD_SIZE_BYTES - 1] ^= 0x01;

    decode_assert_error(bytes, PAYLOAD_ERROR_CRC8_CHECK_FAIL);

    printf("[success] %s()\n", __func__);
}

void test_error_decode_device_type(void) {
    uint8_t bytes[PAYLOAD_SIZE_BYTES];
    copy_bytes_example(bytes);

    bytes[0] ^= 0x10;

    update_crc8(bytes);

    decode_assert_error(bytes, PAYLOAD_ERROR_DEVICE_TYPE_INVALID);

    printf("[success] %s()\n", __func__);
}

void test_error_decode_version(void) {
    uint8_t bytes[PAYLOAD_SIZE_BYTES];
    copy_bytes_example(bytes);

    bytes[0] ^= 0x01;

    update_crc8(bytes);

    decode_assert_error(bytes, PAYLOAD_ERROR_VERSION_INVALID);

    printf("[success] %s()\n", __func__);
}

int main(void) {
    test_decode_encode();

    test_error_decode_crc8();
    test_error_decode_version();
    test_error_decode_device_type();
    test_error_decode_battery_above_maximal();

    return 0;
}
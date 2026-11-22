#include <assert.h>
#include "crc8.h"
#include "payload.h"
#include "payload_constants.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "test.h"

static const uint8_t test_bytes_example[PAYLOAD_SIZE_BYTES] = {
    0x21, 0x51, 0x20, 0xF1, 0x00, 0x10, 0xEF, 0xF1, 0x59, 0x5A, 0x05, 0x65
};

static void test_copy_bytes_example(uint8_t bytes[PAYLOAD_SIZE_BYTES]) {
    memcpy(bytes, test_bytes_example, PAYLOAD_SIZE_BYTES);
}

static payload_t test_copy_payload_example(void) {
    uint8_t bytes[PAYLOAD_SIZE_BYTES];
    test_copy_bytes_example(bytes);

    payload_t payload = {0};
    assert(payload_decode(&payload, bytes) == PAYLOAD_NO_ERROR);

    return payload;
}

static void test_decode_assert_error(
    uint8_t bytes[PAYLOAD_SIZE_BYTES],
    payload_error_t expected_error
) {
    payload_t payload = {0};
    payload_error_t error = payload_decode(&payload, bytes);

    assert(error == expected_error);
}

static void test_encode_assert_error(
    payload_t payload,
    payload_error_t expected_error
) {
    uint8_t bytes[PAYLOAD_SIZE_BYTES] = {0};
    payload_error_t error = payload_encode(payload, bytes);

    assert(error == expected_error);
}

static void test_set_battery_mv(
    uint8_t bytes[PAYLOAD_SIZE_BYTES],
    payload_battery_mv_t battery_mv
) {
    bytes[5] = (uint8_t) ((bytes[5] & 0x03) | ((battery_mv & 0x003F) << 2));
    bytes[6] = (uint8_t) ((bytes[6] & 0xC0) | ((battery_mv >> 6) & 0x003F));
}

static void test_set_flow_exponent(
    uint8_t bytes[PAYLOAD_SIZE_BYTES],
    uint8_t exponent
) {
    uint16_t flow_config = (uint16_t) ((uint16_t)exponent << 8);

    bytes[8] = (uint8_t) ((bytes[8] & 0x01) | ((flow_config & 0x007F) << 1));
    bytes[9] = (uint8_t) ((bytes[9] & 0xE0) | ((flow_config >> 7) & 0x001F));
}

static void test_set_temperature(
    uint8_t bytes[PAYLOAD_SIZE_BYTES],
    payload_temperature_dc_t temperature_dc
) {
    uint16_t temperature_raw = 0;

    if (temperature_dc < 0) {
        temperature_raw = (uint16_t) (temperature_dc + (1 << 10));
    } else {
        temperature_raw = (uint16_t) (temperature_dc);
    }

    bytes[6] = (uint8_t) ((bytes[6] & 0x3F) | ((temperature_raw & 0x0003) << 6));
    bytes[7] = (uint8_t) ((temperature_raw >> 2) & 0x00FF);
    bytes[8] = (uint8_t) ((bytes[8] & 0xFE) | (temperature_dc < 0 ? 0x01 : 0x00));
}

static void test_update_crc8(uint8_t bytes[PAYLOAD_SIZE_BYTES]) {
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
    test_copy_bytes_example(bytes_decode);

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
    test_copy_bytes_example(bytes);

    test_set_battery_mv(
        bytes,
        (payload_battery_mv_t) (PAYLOAD_BATTERY_MAXIMUM_MV + 1U)
    );

    test_update_crc8(bytes);

    test_decode_assert_error(bytes, PAYLOAD_ERROR_BATTERY_ABOVE_MAXIMAL);

    printf("[success] %s()\n", __func__);
}

void test_error_decode_battery_below_minimal(void) {
    uint8_t bytes[PAYLOAD_SIZE_BYTES];
    test_copy_bytes_example(bytes);

    test_set_battery_mv(
        bytes,
        (payload_battery_mv_t) (PAYLOAD_BATTERY_MINIMUM_MV - 1U)
    );

    test_update_crc8(bytes);

    test_decode_assert_error(bytes, PAYLOAD_ERROR_BATTERY_BELOW_MINIMAL);

    printf("[success] %s()\n", __func__);
}

void test_error_decode_crc8(void) {
    uint8_t bytes[PAYLOAD_SIZE_BYTES];
    test_copy_bytes_example(bytes);

    bytes[PAYLOAD_SIZE_BYTES - 1] ^= 0x01;

    test_decode_assert_error(bytes, PAYLOAD_ERROR_CRC8_CHECK_FAIL);

    printf("[success] %s()\n", __func__);
}

void test_error_decode_device_type(void) {
    uint8_t bytes[PAYLOAD_SIZE_BYTES];
    test_copy_bytes_example(bytes);

    bytes[0] ^= 0x10;

    test_update_crc8(bytes);

    test_decode_assert_error(bytes, PAYLOAD_ERROR_DEVICE_TYPE_INVALID);

    printf("[success] %s()\n", __func__);
}

void test_error_decode_flow_exponent_invalid(void) {
    uint8_t bytes[PAYLOAD_SIZE_BYTES];
    test_copy_bytes_example(bytes);

    test_set_flow_exponent(bytes, 0x0F);

    test_update_crc8(bytes);

    test_decode_assert_error(bytes, PAYLOAD_ERROR_FLOW_EXPONENT_INVALID);

    printf("[success] %s()\n", __func__);
}

void test_error_decode_reserved(void) {
    uint8_t bytes[PAYLOAD_SIZE_BYTES];
    test_copy_bytes_example(bytes);

    uint8_t invalid_reserved = (
        (uint8_t) PAYLOAD_RESERVED_EXPECTED ^ 0x01
    ) & 0x07;

    bytes[10] = (uint8_t) (
        (bytes[10] & 0x1F) | (invalid_reserved << 5)
    );

    test_update_crc8(bytes);

    test_decode_assert_error(bytes, PAYLOAD_ERROR_RESERVED_INVALID);

    printf("[success] %s()\n", __func__);
}

void test_error_decode_temperature_above_maximal(void) {
    uint8_t bytes[PAYLOAD_SIZE_BYTES];
    test_copy_bytes_example(bytes);

    test_set_temperature(
        bytes,
        (payload_temperature_dc_t) (PAYLOAD_TEMPERATURE_MAXIMUM_DC + 1U)
    );

    test_update_crc8(bytes);

    test_decode_assert_error(bytes, PAYLOAD_ERROR_TEMPERATURE_ABOVE_MAXIMAL);

    printf("[success] %s()\n", __func__);
}

void test_error_decode_temperature_below_minimal(void) {
    uint8_t bytes[PAYLOAD_SIZE_BYTES];
    test_copy_bytes_example(bytes);

    test_set_temperature(
        bytes,
        (payload_temperature_dc_t) (PAYLOAD_TEMPERATURE_MINIMUM_DC - 1U)
    );

    test_update_crc8(bytes);

    test_decode_assert_error(bytes, PAYLOAD_ERROR_TEMPERATURE_BELOW_MINIMAL);

    printf("[success] %s()\n", __func__);
}

void test_error_decode_version(void) {
    uint8_t bytes[PAYLOAD_SIZE_BYTES];
    test_copy_bytes_example(bytes);

    bytes[0] ^= 0x01;

    test_update_crc8(bytes);

    test_decode_assert_error(bytes, PAYLOAD_ERROR_VERSION_INVALID);

    printf("[success] %s()\n", __func__);
}

void test_error_encode_battery_above_maximal(void) {
    payload_t payload = test_copy_payload_example();

    payload.battery_mv = (
        (payload_battery_mv_t) (PAYLOAD_BATTERY_MAXIMUM_MV + 1U)
    );

    test_encode_assert_error(payload, PAYLOAD_ERROR_BATTERY_ABOVE_MAXIMAL);

    printf("[success] %s()\n", __func__);
}

void test_error_encode_battery_below_minimal(void) {
    payload_t payload = test_copy_payload_example();

    payload.battery_mv = (
        (payload_battery_mv_t) (PAYLOAD_BATTERY_MINIMUM_MV - 1U)
    );

    test_encode_assert_error(payload, PAYLOAD_ERROR_BATTERY_BELOW_MINIMAL);

    printf("[success] %s()\n", __func__);
}

void test_error_encode_device_type(void) {
    payload_t payload = test_copy_payload_example();

    payload.device_type = (payload_device_type_t) (payload.device_type ^ 0x01);

    test_encode_assert_error(payload, PAYLOAD_ERROR_DEVICE_TYPE_INVALID);

    printf("[success] %s()\n", __func__);
}

void test_error_encode_flow_exponent_invalid(void) {
    payload_t payload = test_copy_payload_example();

    // 1024 * (1 + 255 / 256) = 2044 L/h
    payload.flow_lph = 2048.0f;

    test_encode_assert_error(payload, PAYLOAD_ERROR_FLOW_EXPONENT_INVALID);

    printf("[success] %s()\n", __func__);
}

void test_error_encode_reserved(void) {
    payload_t payload = test_copy_payload_example();

    payload.reserved = (
        (payload_reserved_t) (
            (((uint8_t) PAYLOAD_RESERVED_EXPECTED) ^ 0x01) & 0x07
        )
    );

    test_encode_assert_error(payload, PAYLOAD_ERROR_RESERVED_INVALID);

    printf("[success] %s()\n", __func__);
}

void test_error_encode_temperature_above_maximal(void) {
    payload_t payload = test_copy_payload_example();

    payload.temperature_dc = (
        (payload_temperature_dc_t) (PAYLOAD_TEMPERATURE_MAXIMUM_DC + 1)
    );

    test_encode_assert_error(payload, PAYLOAD_ERROR_TEMPERATURE_ABOVE_MAXIMAL);

    printf("[success] %s()\n", __func__);
}

void test_error_encode_temperature_below_minimal(void) {
    payload_t payload = test_copy_payload_example();

    payload.temperature_dc = (
        (payload_temperature_dc_t) (PAYLOAD_TEMPERATURE_MINIMUM_DC - 1)
    );

    test_encode_assert_error(payload, PAYLOAD_ERROR_TEMPERATURE_BELOW_MINIMAL);

    printf("[success] %s()\n", __func__);
}

void test_error_encode_version(void) {
    payload_t payload = test_copy_payload_example();

    payload.version = (payload_version_t) (payload.version ^ 0x01);

    test_encode_assert_error(payload, PAYLOAD_ERROR_VERSION_INVALID);

    printf("[success] %s()\n", __func__);
}

void test_flags_combinations(void) {
    payload_t payload = test_copy_payload_example();

    for (uint8_t flags = 0; flags < 8; flags++) {
        payload.flags.battery_low   = (bool) ((flags >> 0) & 0x01);
        payload.flags.violation     = (bool) ((flags >> 1) & 0x01);
        payload.flags.flow_reversed = (bool) ((flags >> 2) & 0x01);

        uint8_t bytes[PAYLOAD_SIZE_BYTES] = {0};
        assert(payload_encode(payload, bytes) == PAYLOAD_NO_ERROR);

        payload_t payload_d = {0};
        assert(payload_decode(&payload_d, bytes) == PAYLOAD_NO_ERROR);

        assert(payload_d.flags.battery_low    == payload.flags.battery_low);
        assert(payload_d.flags.violation      == payload.flags.violation);
        assert(payload_d.flags.flow_reversed  == payload.flags.flow_reversed);
    }

    printf("[success] %s()\n", __func__);
}

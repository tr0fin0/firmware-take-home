#include "crc8.h"
#include <inttypes.h>
#include "payload.h"
#include "payload_constants.h"
#include <stdio.h>

static const char *payload_error_messages[NUMBER_OF_PAYLOAD_ERRORS] = {
    [PAYLOAD_NO_ERROR]
        = "payload without errors",

    [PAYLOAD_ERROR_BATTERY_ABOVE_MAXIMAL]
        = "payload with battery above maximal",

    [PAYLOAD_ERROR_BATTERY_BELOW_MINIMAL]
        = "payload with battery below minimal",

    [PAYLOAD_ERROR_CRC8_CHECK_FAIL]
        = "payload with CRC8 check failure",

    [PAYLOAD_ERROR_DEVICE_TYPE_INVALID]
        = "payload with invalid device type",

    [PAYLOAD_ERROR_FLOW_EXPONENT_INVALID]
        = "payload with invalid flow exponent",

    [PAYLOAD_ERROR_RESERVED_INVALID]
        = "payload with invalid reserved bits",

    [PAYLOAD_ERROR_TEMPERATURE_ABOVE_MAXIMAL]
        = "payload with temperature above maximal",

    [PAYLOAD_ERROR_TEMPERATURE_BELOW_MINIMAL]
        = "payload with temperature below minimal",

    [PAYLOAD_ERROR_VERSION_INVALID]
        = "payload with invalid version",
};

/**
 * @brief Reads a bit field from the payload byte array.
 *
 * Extracts the bits between the initial and final bit indexes inclusive and
 * returns the resulting value, with the initial bit as the LSB.
 *
 * @param bytes Payload byte array.
 * @param i_bit_start Absolute index of the first bit to read.
 * @param i_bit_finish Absolute index of the last bit to read.
 *
 * @return Extracted bit field.
 *
 * @note Supports bit fields of up to 32 bits.
 * @note The caller must provide valid indexes within the payload array.
 */
static uint32_t payload_bits_read(
    const uint8_t bytes[PAYLOAD_SIZE_BYTES],
    uint16_t i_bit_start,
    uint16_t i_bit_finish
) {
    uint32_t value = 0;

    for (uint16_t i_bit = i_bit_start; i_bit <= i_bit_finish; i_bit++) {
        uint16_t byte_index = i_bit / 8U;
        uint8_t bit_offset  = (uint8_t) (i_bit % 8U);

        uint32_t bit_value   = (uint32_t) (
            (bytes[byte_index] >> bit_offset) & 0x01U
        );

        value |= bit_value << (i_bit - i_bit_start);
    }

    return value;
}

/**
 * @brief Writes a bit field into the payload byte array.
 *
 * Writes the bits of the provided value between the initial and final
 * bit indexes (inclusive), with the initial bit receiving the LSB.
 *
 * Existing bits outside the specified range are preserved.
 *
 * @param bytes Payload byte array.
 * @param i_bit_start Absolute index of the first bit to write.
 * @param i_bit_finish Absolute index of the last bit to write.
 * @param value Value to write.
 *
 * @note Supports bit fields of up to 32 bits.
 * @note The caller must provide valid indexes within the payload array.
 * @note Bits exceeding the specified field width are discarded.
 */
static void payload_bits_write(
    uint8_t bytes[PAYLOAD_SIZE_BYTES],
    uint16_t i_bit_start,
    uint16_t i_bit_finish,
    uint32_t value
) {
    for (uint16_t i_bit = i_bit_start; i_bit <= i_bit_finish; i_bit++) {
        uint16_t byte_index = i_bit / 8U;
        uint8_t bit_offset  = (uint8_t) (i_bit % 8U);

        uint32_t value_bit_index    = i_bit - i_bit_start;
        uint8_t bit_mask            = (uint8_t) (1U << bit_offset);

        uint8_t bit_value = (uint8_t) (
            (value >> value_bit_index) & 0x01U
        );

        bytes[byte_index] = (uint8_t) (
            (bytes[byte_index] & (uint8_t) ~bit_mask) |
            ((uint8_t) (bit_value << bit_offset))
        );
    }
}

static payload_error_t payload_decode_battery(
    payload_t *payload, uint8_t bytes[PAYLOAD_SIZE_BYTES]
) {
    payload_battery_mv_t battery_mv = (
        (payload_battery_mv_t) payload_bits_read(bytes, 42U, 53U)
    );

    if (battery_mv > PAYLOAD_BATTERY_MAXIMUM_MV) {
        return PAYLOAD_ERROR_BATTERY_ABOVE_MAXIMAL;
    }

    if (battery_mv < PAYLOAD_BATTERY_MINIMUM_MV) {
        return PAYLOAD_ERROR_BATTERY_BELOW_MINIMAL;
    }

    payload->battery_mv = battery_mv;
    return PAYLOAD_NO_ERROR;
}

static payload_error_t payload_decode_crc8(
    payload_t *payload, uint8_t bytes[PAYLOAD_SIZE_BYTES]
) {
    crc8_config_t crc8_config = {
        .init_value     = 0x00,
        .polynomial     = 0x07,
        .reflection_in  = false,
        .reflection_out = false,
        .xor_out        = 0x00
    };

    crc8_value_t crc8_value = crc8_compute(
        crc8_config, bytes, PAYLOAD_SIZE_BYTES - 1
    );

    if (crc8_value != (crc8_value_t) bytes[PAYLOAD_SIZE_BYTES - 1]) {
        return PAYLOAD_ERROR_CRC8_CHECK_FAIL;
    }

    payload->crc8_value = crc8_value;
    return PAYLOAD_NO_ERROR;
}

static payload_error_t payload_decode_device_type(
    payload_t *payload, uint8_t bytes[PAYLOAD_SIZE_BYTES]
) {
    payload_device_type_t device_type = (
        (payload_device_type_t) payload_bits_read(bytes, 4U, 11U)
    );

    if (device_type != PAYLOAD_DEVICE_TYPE_1) {
        return PAYLOAD_ERROR_DEVICE_TYPE_INVALID;
    }

    payload->device_type = device_type;
    return PAYLOAD_NO_ERROR;
}

static payload_error_t payload_decode_flags(
    payload_t *payload, uint8_t bytes[PAYLOAD_SIZE_BYTES]
) {
    payload_flags_t decoded_flags = {0};

    decoded_flags.battery_low       = payload_bits_read(bytes, 12U, 12U) != 0U;
    decoded_flags.violation         = payload_bits_read(bytes, 13U, 13U) != 0U;
    decoded_flags.flow_reversed     = payload_bits_read(bytes, 14U, 14U) != 0U;

    payload->flags.battery_low      = decoded_flags.battery_low;
    payload->flags.violation        = decoded_flags.violation;
    payload->flags.flow_reversed    = decoded_flags.flow_reversed;
    return PAYLOAD_NO_ERROR;
}

static const float flow_exponents[] = {
    0.125f,  // 2^-3
    0.25f,   // 2^-2
    0.5f,    // 2^-1
    1.0f,    // 2^0
    2.0f,    // 2^1
    4.0f,    // 2^2
    8.0f,    // 2^3
    16.0f,   // 2^4
    32.0f,   // 2^5
    64.0f,   // 2^6
    128.0f,  // 2^7
    256.0f,  // 2^8
    512.0f,  // 2^9
    1024.0f  // 2^10
};

static payload_error_t payload_decode_flow(
    payload_t *payload, uint8_t bytes[PAYLOAD_SIZE_BYTES]
) {
    uint16_t flow_config = (
        (uint16_t) payload_bits_read(bytes, 65U, 76U)
    );

    uint8_t mantissa = (uint8_t) ( flow_config & 0x00FF);
    uint8_t exponent = (uint8_t) ((flow_config & 0x0F00) >> 8);

    if (exponent == 0x0F) {
        return PAYLOAD_ERROR_FLOW_EXPONENT_INVALID;
    }

    payload_flow_lph_t flow_lph = 0;
    if (exponent == 0x00) {
        flow_lph = (payload_flow_lph_t) (
            ((float) mantissa) / (1024.0f)
        );
    } else {
        flow_lph = (payload_flow_lph_t) (
            (1.0f + ((float) mantissa / 256.0f)) * (flow_exponents[exponent])
        );
    }

    payload->flow_lph = flow_lph;
    return PAYLOAD_NO_ERROR;
}

static payload_error_t payload_decode_pulse_count(
    payload_t *payload, uint8_t bytes[PAYLOAD_SIZE_BYTES]
) {
    payload_pulse_count_t pulse_count = (
        (payload_pulse_count_t) payload_bits_read(bytes, 15U, 41U)
    );

    payload->pulse_count = pulse_count;
    return PAYLOAD_NO_ERROR;
}

static payload_error_t payload_decode_reserved(
    payload_t *payload, uint8_t bytes[PAYLOAD_SIZE_BYTES]
) {
    payload_reserved_t reserved = (
        (payload_reserved_t) payload_bits_read(bytes, 85U, 87U)
    );

    if (reserved != PAYLOAD_RESERVED_EXPECTED) {
        return PAYLOAD_ERROR_RESERVED_INVALID;
    }

    payload->reserved = reserved;
    return PAYLOAD_NO_ERROR;
}

static payload_error_t payload_decode_sequence(
    payload_t *payload, uint8_t bytes[PAYLOAD_SIZE_BYTES]
) {
    payload_sequence_t sequence = (
        (payload_sequence_t) payload_bits_read(bytes, 77U, 84U)
    );

    payload->sequence = sequence;
    return PAYLOAD_NO_ERROR;
}

static payload_error_t payload_decode_temperature(
    payload_t *payload, uint8_t bytes[PAYLOAD_SIZE_BYTES]
) {
    payload_temperature_dc_t temperature_dc = (
        (payload_temperature_dc_t) payload_bits_read(bytes, 54U, 63U)
    );

    bool temperature_negative = (payload_bits_read(bytes, 64U, 64U) != 0U);
    if (temperature_negative) {
        temperature_dc -= (1 << 10);
    }

    if (temperature_dc > PAYLOAD_TEMPERATURE_MAXIMUM_DC) {
        return PAYLOAD_ERROR_TEMPERATURE_ABOVE_MAXIMAL;
    }

    if (temperature_dc < PAYLOAD_TEMPERATURE_MINIMUM_DC) {
        return PAYLOAD_ERROR_TEMPERATURE_BELOW_MINIMAL;
    }

    payload->temperature_dc = temperature_dc;
    return PAYLOAD_NO_ERROR;
}

static payload_error_t payload_decode_version(
    payload_t *payload, uint8_t bytes[PAYLOAD_SIZE_BYTES]
) {
    payload_version_t version = (
        (payload_version_t) payload_bits_read(bytes, 0U, 3U)
    );

    if (version != PAYLOAD_VERSION_1) {
        return PAYLOAD_ERROR_VERSION_INVALID;
    }

    payload->version = version;
    return PAYLOAD_NO_ERROR;
}

static payload_error_t payload_encode_battery(
    payload_t payload, uint8_t bytes[PAYLOAD_SIZE_BYTES]
) {
    payload_battery_mv_t battery_mv = payload.battery_mv;

    if (battery_mv > PAYLOAD_BATTERY_MAXIMUM_MV) {
        return PAYLOAD_ERROR_BATTERY_ABOVE_MAXIMAL;
    }

    if (battery_mv < PAYLOAD_BATTERY_MINIMUM_MV) {
        return PAYLOAD_ERROR_BATTERY_BELOW_MINIMAL;
    }

    payload_bits_write(bytes, 42U, 53U, (uint32_t) battery_mv);

    return PAYLOAD_NO_ERROR;
}

static payload_error_t payload_encode_crc8(uint8_t bytes[PAYLOAD_SIZE_BYTES]) {
    crc8_config_t crc8_config = {
        .init_value     = 0x00,
        .polynomial     = 0x07,
        .reflection_in  = false,
        .reflection_out = false,
        .xor_out        = 0x00
    };

    crc8_value_t crc8_value = crc8_compute(
        crc8_config, bytes, PAYLOAD_SIZE_BYTES - 1
    );

    bytes[PAYLOAD_SIZE_BYTES - 1] = crc8_value;
    return PAYLOAD_NO_ERROR;
}

static payload_error_t payload_encode_device_type(
    payload_t payload, uint8_t bytes[PAYLOAD_SIZE_BYTES]
) {
    payload_device_type_t device_type = payload.device_type;

    if (device_type != PAYLOAD_DEVICE_TYPE_1) {
        return PAYLOAD_ERROR_DEVICE_TYPE_INVALID;
    }

    payload_bits_write(bytes, 4U, 11U, (uint32_t) device_type);

    return PAYLOAD_NO_ERROR;
}

static payload_error_t payload_encode_flags(
    payload_t payload, uint8_t bytes[PAYLOAD_SIZE_BYTES]
) {
    payload_bits_write(
        bytes, 12U, 12U, (uint32_t) (payload.flags.battery_low   ? 1U : 0U)
    );
    payload_bits_write(
        bytes, 13U, 13U, (uint32_t) (payload.flags.violation     ? 1U : 0U)
    );
    payload_bits_write(
        bytes, 14U, 14U, (uint32_t) (payload.flags.flow_reversed ? 1U : 0U)
    );

    return PAYLOAD_NO_ERROR;
}

static payload_error_t payload_encode_flow(
    payload_t payload, uint8_t bytes[PAYLOAD_SIZE_BYTES]
) {
    payload_flow_lph_t flow_lph = payload.flow_lph;

    uint8_t flow_exponent = 0;
    uint8_t flow_mantissa = 0;

    if (flow_lph < 0.0f) {
        return PAYLOAD_ERROR_FLOW_EXPONENT_INVALID;
    }

    if (flow_lph < flow_exponents[1]) {
        uint16_t rounded_mantissa = (uint16_t) ((flow_lph * 1024.0f) + 0.5f);

        if (rounded_mantissa > 0xFF) {
            flow_exponent = 1;
            flow_mantissa = 0;
        } else {
            flow_exponent = 0;
            flow_mantissa = (uint8_t) rounded_mantissa;
        }
    } else {
        const uint8_t num_flow_exponents = (
            (uint8_t) (sizeof(flow_exponents) / sizeof(flow_exponents[0]))
        );
        
        bool exponent_found = false;
        for (uint8_t i = 1; i < num_flow_exponents; i++) {
            float i_exponent = flow_exponents[i];

            if (flow_lph < (i_exponent * 2.0f)) {
                float mantissa_value = (
                    ((flow_lph / i_exponent) - 1.0f) * 256.0f
                );

                uint16_t rounded_mantissa = (uint16_t) (mantissa_value + 0.5f);

                if (rounded_mantissa > 0xFF) {
                    if ((uint8_t) (i + 1) >= num_flow_exponents) {
                        return PAYLOAD_ERROR_FLOW_EXPONENT_INVALID;
                    }

                    flow_exponent = (uint8_t) (i + 1);
                    flow_mantissa = 0;
                } else {
                    flow_exponent = i;
                    flow_mantissa = (uint8_t) rounded_mantissa;
                }

                exponent_found = true;
                break;
            }
        }

        if (!exponent_found) {
            return PAYLOAD_ERROR_FLOW_EXPONENT_INVALID;
        }
    }

    uint16_t flow_config = 0;

    flow_config |= (uint16_t) flow_mantissa;
    flow_config |= (uint16_t) (((uint16_t) flow_exponent) << 8);

    payload_bits_write(bytes, 65U, 76U, (uint32_t) flow_config);

    return PAYLOAD_NO_ERROR;
}

static payload_error_t payload_encode_pulse_count(
    payload_t payload, uint8_t bytes[PAYLOAD_SIZE_BYTES]
) {
    uint32_t pulse_count = (uint32_t) payload.pulse_count;

    payload_bits_write(bytes, 15U, 41U, (uint32_t) pulse_count);

    return PAYLOAD_NO_ERROR;
}

static payload_error_t payload_encode_reserved(
    payload_t payload, uint8_t bytes[PAYLOAD_SIZE_BYTES]
) {
    payload_reserved_t reserved = payload.reserved;

    if (reserved != PAYLOAD_RESERVED_EXPECTED) {
        return PAYLOAD_ERROR_RESERVED_INVALID;
    }

    payload_bits_write(bytes, 85U, 87U, (uint32_t) reserved);

    return PAYLOAD_NO_ERROR;
}

static payload_error_t payload_encode_sequence(
    payload_t payload, uint8_t bytes[PAYLOAD_SIZE_BYTES]
) {
    payload_sequence_t sequence = payload.sequence;

    payload_bits_write(bytes, 77U, 84U, (uint32_t) sequence);

    return PAYLOAD_NO_ERROR;
}

static payload_error_t payload_encode_temperature(
    payload_t payload, uint8_t bytes[PAYLOAD_SIZE_BYTES]
) {
    payload_temperature_dc_t temperature_dc = payload.temperature_dc;

    if (temperature_dc > PAYLOAD_TEMPERATURE_MAXIMUM_DC) {
        return PAYLOAD_ERROR_TEMPERATURE_ABOVE_MAXIMAL;
    }

    if (temperature_dc < PAYLOAD_TEMPERATURE_MINIMUM_DC) {
        return PAYLOAD_ERROR_TEMPERATURE_BELOW_MINIMAL;
    }

    uint16_t temperature_raw = 0;
    if (temperature_dc < 0) {
        temperature_raw = (uint16_t) (temperature_dc + (1 << 10));
    } else {
        temperature_raw = (uint16_t) (temperature_dc);
    }

    payload_bits_write(
        bytes, 54U, 63U, (uint32_t) temperature_raw
    );
    payload_bits_write(
        bytes, 64U, 64U, (uint32_t) (temperature_dc < 0 ? 1U : 0U)
    );

    return PAYLOAD_NO_ERROR;
}

static payload_error_t payload_encode_version(
    payload_t payload, uint8_t bytes[PAYLOAD_SIZE_BYTES]
) {
    payload_version_t version = payload.version;

    if (version != PAYLOAD_VERSION_1) {
        return PAYLOAD_ERROR_VERSION_INVALID;
    }

    payload_bits_write(bytes, 0U, 3U, (uint32_t) version);

    return PAYLOAD_NO_ERROR;
}

payload_error_t payload_decode(
    payload_t *payload, uint8_t bytes[PAYLOAD_SIZE_BYTES]
) {
    payload_error_t error;

    error = payload_decode_crc8(payload, bytes);
    if (error != PAYLOAD_NO_ERROR) {
        return error;
    }

    error = payload_decode_version(payload, bytes);
    if (error != PAYLOAD_NO_ERROR) {
        return error;
    }

    error = payload_decode_device_type(payload, bytes);
    if (error != PAYLOAD_NO_ERROR) {
        return error;
    }

    error = payload_decode_flags(payload, bytes);
    if (error != PAYLOAD_NO_ERROR) {
        return error;
    }

    error = payload_decode_pulse_count(payload, bytes);
    if (error != PAYLOAD_NO_ERROR) {
        return error;
    }

    error = payload_decode_battery(payload, bytes);
    if (error != PAYLOAD_NO_ERROR) {
        return error;
    }

    error = payload_decode_temperature(payload, bytes);
    if (error != PAYLOAD_NO_ERROR) {
        return error;
    }

    error = payload_decode_flow(payload, bytes);
    if (error != PAYLOAD_NO_ERROR) {
        return error;
    }

    error = payload_decode_sequence(payload, bytes);
    if (error != PAYLOAD_NO_ERROR) {
        return error;
    }

    error = payload_decode_reserved(payload, bytes);
    if (error != PAYLOAD_NO_ERROR) {
        return error;
    }

    return PAYLOAD_NO_ERROR;
}

payload_error_t payload_encode(
    payload_t payload, uint8_t bytes[PAYLOAD_SIZE_BYTES]
) {
    payload_error_t error;

    error = payload_encode_version(payload, bytes);
    if (error != PAYLOAD_NO_ERROR) {
        return error;
    }

    error = payload_encode_device_type(payload, bytes);
    if (error != PAYLOAD_NO_ERROR) {
        return error;
    }

    error = payload_encode_flags(payload, bytes);
    if (error != PAYLOAD_NO_ERROR) {
        return error;
    }

    error = payload_encode_pulse_count(payload, bytes);
    if (error != PAYLOAD_NO_ERROR) {
        return error;
    }

    error = payload_encode_battery(payload, bytes);
    if (error != PAYLOAD_NO_ERROR) {
        return error;
    }

    error = payload_encode_temperature(payload, bytes);
    if (error != PAYLOAD_NO_ERROR) {
        return error;
    }

    error = payload_encode_flow(payload, bytes);
    if (error != PAYLOAD_NO_ERROR) {
        return error;
    }

    error = payload_encode_sequence(payload, bytes);
    if (error != PAYLOAD_NO_ERROR) {
        return error;
    }

    error = payload_encode_reserved(payload, bytes);
    if (error != PAYLOAD_NO_ERROR) {
        return error;
    }

    payload_encode_crc8(bytes);

    return PAYLOAD_NO_ERROR;
}

void payload_print(payload_t *payload) {
    if (payload == NULL) {
        return;
    }

    printf(
        "version:         %d\n"
        "device_type:     %d\n"
        "flags:\n"
        "  battery_low:   %s\n"
        "  violation:     %s\n"
        "  flow_reversed: %s\n"
        "pulse_count:     %" PRIu32 "\n"
        "battery:         %04u mV\n"
        "temperature:     %d deci-C\n"
        "flow:            %04f L/h\n"
        "sequence:        %u\n"
        "reserved:        0x%02x\n"
        "crc8:            0x%02x\n",
        (payload_version_t)         payload->version,
        (payload_device_type_t)     payload->device_type,
        payload->flags.battery_low      ? "true" : "false",
        payload->flags.violation        ? "true" : "false",
        payload->flags.flow_reversed    ? "true" : "false",
        (payload_pulse_count_t)     payload->pulse_count,
        (payload_battery_mv_t)      payload->battery_mv,
        (payload_temperature_dc_t)  payload->temperature_dc,
        (payload_flow_lph_t)        payload->flow_lph,
        (payload_sequence_t)        payload->sequence,
        (payload_reserved_t)        payload->reserved,
        (crc8_value_t)              payload->crc8_value
    );
}

void payload_print_error(payload_error_t error) {
    if ((unsigned) error >= NUMBER_OF_PAYLOAD_ERRORS) {
        return;
    }

    printf("%s\n", payload_error_messages[error]);
}


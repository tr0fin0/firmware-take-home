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

    [PAYLOAD_ERROR_FLAG_INVALID]
        = "payload with invalid flag",

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

static payload_error_t payload_decode_battery(
    payload_t *payload, uint8_t bytes[PAYLOAD_SIZE_BYTES]
) {
    payload_battery_mv_t battery_mv = 0;

    battery_mv |= (payload_battery_mv_t)((bytes[5] & 0xFC) >> 2);
    battery_mv |= (payload_battery_mv_t)((bytes[6] & 0x3F) << 6);

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
    payload_device_type_t device_type = 0;

    device_type |= (payload_device_type_t) ((bytes[0] & 0xF0) >> 4);
    device_type |= (payload_device_type_t) ((bytes[1] & 0x0F) << 4);

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

    decoded_flags.battery_low   = (bool) (((bytes[1] & 0x10) >> 4) == 1);
    decoded_flags.violation     = (bool) (((bytes[1] & 0x20) >> 5) == 1);
    decoded_flags.flow_reversed = (bool) (((bytes[1] & 0x40) >> 6) == 1);

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
    uint16_t flow_config = 0;

    flow_config |= (uint16_t) ((bytes[8] & 0xFE) >> 1);
    flow_config |= (uint16_t) ((bytes[9] & 0x1F) << 7);

    uint8_t mantissa = (uint8_t) (flow_config & 0x00FF);
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
    payload_pulse_count_t pulse_count = 0;

    pulse_count |= (payload_pulse_count_t) ((bytes[1] & 0x80) >>  7);
    pulse_count |= (payload_pulse_count_t) ((bytes[2] & 0xFF) <<  1);
    pulse_count |= (payload_pulse_count_t) ((bytes[3] & 0xFF) <<  9);
    pulse_count |= (payload_pulse_count_t) ((bytes[4] & 0xFF) << 17);
    pulse_count |= (payload_pulse_count_t) ((bytes[5] & 0x07) << 25);

    payload->pulse_count = pulse_count;
    return PAYLOAD_NO_ERROR;
}

static payload_error_t payload_decode_reserved(
    payload_t *payload, uint8_t bytes[PAYLOAD_SIZE_BYTES]
) {
    payload_reserved_t reserved = 0;

    reserved |= (payload_reserved_t)((bytes[10] & 0xE0) >> 5);

    if (reserved != PAYLOAD_RESERVED_EXPECTED) {
        return PAYLOAD_ERROR_RESERVED_INVALID;
    }

    payload->reserved = reserved;
    return PAYLOAD_NO_ERROR;
}

static payload_error_t payload_decode_sequence(
    payload_t *payload, uint8_t bytes[PAYLOAD_SIZE_BYTES]
) {
    payload_sequence_t sequence = 0;

    sequence |= (payload_sequence_t)((bytes[9]  & 0xE0) >> 5);
    sequence |= (payload_sequence_t)((bytes[10] & 0x1F) << 3);

    payload->sequence = sequence;
    return PAYLOAD_NO_ERROR;
}

static payload_error_t payload_decode_temperature(
    payload_t *payload, uint8_t bytes[PAYLOAD_SIZE_BYTES]
) {
    payload_temperature_dc_t temperature_dc = 0;

    temperature_dc |= (payload_temperature_dc_t)((bytes[6] & 0xC0) >> 6);
    temperature_dc |= (payload_temperature_dc_t)((bytes[7] & 0xFF) << 2);

    bool temperature_negative = (bool) ((bytes[8] & 0x01) == 1);
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
    payload_version_t version = (payload_version_t) (bytes[0] & 0x0F);

    if (version != PAYLOAD_VERSION_1) {
        return PAYLOAD_ERROR_VERSION_INVALID;
    }

    payload->version = version;
    return PAYLOAD_NO_ERROR;
}

static payload_error_t payload_encode_device_type(
    payload_t payload, uint8_t bytes[PAYLOAD_SIZE_BYTES]
) {
    payload_device_type_t device_type = payload.device_type;

    if (device_type != PAYLOAD_DEVICE_TYPE_1) {
        return PAYLOAD_ERROR_DEVICE_TYPE_INVALID;
    }

    bytes[0] |= (uint8_t) ((device_type & 0x0F) << 4);
    bytes[1] |= (uint8_t) ((device_type >> 4) & 0x0F);

    return PAYLOAD_NO_ERROR;
}

static payload_error_t payload_encode_flags(
    payload_t payload, uint8_t bytes[PAYLOAD_SIZE_BYTES]
) {
    bytes[1] |= (uint8_t) ((payload.flags.battery_low   ? 1U : 0U) << 4);
    bytes[1] |= (uint8_t) ((payload.flags.violation     ? 1U : 0U) << 5);
    bytes[1] |= (uint8_t) ((payload.flags.flow_reversed ? 1U : 0U) << 6);

    return PAYLOAD_NO_ERROR;
}
static payload_error_t payload_encode_pulse_count(
    payload_t payload, uint8_t bytes[PAYLOAD_SIZE_BYTES]
) {
    uint32_t pulse_count = (uint32_t) payload.pulse_count;

    bytes[1] |= (uint8_t) ((pulse_count & 0x00000001) << 7);
    bytes[2] |= (uint8_t) ((pulse_count >> 1)  & 0x000000FF);
    bytes[3] |= (uint8_t) ((pulse_count >> 9)  & 0x000000FF);
    bytes[4] |= (uint8_t) ((pulse_count >> 17) & 0x000000FF);
    bytes[5] |= (uint8_t) ((pulse_count >> 25) & 0x00000007);

    return PAYLOAD_NO_ERROR;
}


static payload_error_t payload_encode_version(
    payload_t payload, uint8_t bytes[PAYLOAD_SIZE_BYTES]
) {
    payload_version_t version = payload.version;

    if (version != PAYLOAD_VERSION_1) {
        return PAYLOAD_ERROR_VERSION_INVALID;
    }

    bytes[0] |= (uint8_t) (version & 0x0F);

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


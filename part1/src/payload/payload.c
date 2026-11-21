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

static payload_error_t payload_decode_version(
    payload_t *payload, uint8_t  bytes[PAYLOAD_SIZE_BYTES]
) {
    payload_version_t version = (payload_version_t) (bytes[0] & 0x0F);

    if (version != PAYLOAD_VERSION_1) {
        return PAYLOAD_ERROR_VERSION_INVALID;
    }

    payload->version = version;
    return PAYLOAD_NO_ERROR;
}

static payload_error_t payload_decode_device_type(
    payload_t *payload, uint8_t  bytes[PAYLOAD_SIZE_BYTES]
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
    payload_t *payload, uint8_t  bytes[PAYLOAD_SIZE_BYTES]
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

static payload_error_t payload_decode_pulse_count(
    payload_t *payload, uint8_t  bytes[PAYLOAD_SIZE_BYTES]
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

static payload_error_t payload_decode_battery(
    payload_t *payload, uint8_t  bytes[PAYLOAD_SIZE_BYTES]
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

static payload_error_t payload_decode_temperature(
    payload_t *payload, uint8_t  bytes[PAYLOAD_SIZE_BYTES]
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

payload_error_t payload_decode(
    payload_t *payload, uint8_t  bytes[PAYLOAD_SIZE_BYTES]
) {
    payload_error_t error;

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

    return PAYLOAD_NO_ERROR;
}

payload_error_t payload_encode(
    payload_t  payload, uint8_t *bytes[PAYLOAD_SIZE_BYTES]
) {

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


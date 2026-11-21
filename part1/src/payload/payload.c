#include <inttypes.h>
#include "payload.h"
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


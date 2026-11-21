#include <inttypes.h>
#include "payload.h"
#include <stdio.h>

payload_error_t payload_decode(
    payload_t *payload, uint8_t  bytes[PAYLOAD_SIZE_BYTES]
) {

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
        (crc8_value_t)              payload->crc8_check
    );
}

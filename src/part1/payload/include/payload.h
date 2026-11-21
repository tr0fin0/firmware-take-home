/**
 * @file payload.h
 * @brief BLE payload decoding and encoding.
 *
 * @author Guilherme Nunes Trofino
 * @date 2026-11-21
 */

#pragma once

#include "crc8.h"
#include "payload_types.h"

/**
 * @brief 
 */
typedef struct payload {
    /**
     * @brief 
     */
    payload_battery_mv_t battery_mv;

    /**
     * @brief 
     */
    crc8_value_t crc8_check;

    /**
     * @brief 
     */
    payload_device_type_t device_type;

    /**
     * @brief 
     */
    payload_flags_t flags;

    /**
     * @brief 
     */
    payload_flow_lph_t flow;

    /**
     * @brief 
     */
    payload_pulse_count_t pulse_count;

    /**
     * @brief 
     */
    payload_reserved_t reserved;

    /**
     * @brief 
     */
    payload_sequence_t sequence;

    /**
     * @brief 
     */
    payload_temperature_dc_t temperature_dc;

    /**
     * @brief 
     */
    payload_version_t version;
} payload_t;

/**
 * @brief 
 */
#define PAYLOAD_SIZE_BYTES 12

/**
 * @brief 
 */
payload_error_t payload_decode(
    payload_t *payload, uint8_t  bytes[PAYLOAD_SIZE_BYTES]
);

/**
 * @brief 
 */
payload_error_t payload_encode(
    payload_t  payload, uint8_t *bytes[PAYLOAD_SIZE_BYTES]
);

/**
 * @brief 
 */
payload_error_t payload_get_battery_mv(
    payload_t *payload, payload_battery_mv_t *battery
);

/**
 * @brief 
 */
payload_error_t payload_get_device_type(
    payload_t *payload, payload_device_type_t *device_type
);

/**
 * @brief 
 */
payload_error_t payload_get_flag(
    payload_t *payload, payload_flag_t flag, bool *status
);

/**
 * @brief 
 */
payload_error_t payload_get_flow_lph(
    payload_t *payload, payload_flow_lph_t *flow_lph
);

/**
 * @brief 
 */
payload_error_t payload_get_pulse_count(
    payload_t *payload, payload_pulse_count_t *pulse_count
);

/**
 * @brief 
 */
payload_error_t payload_get_reserved(
    payload_t *payload, payload_reserved_t *reserved
);

/**
 * @brief 
 */
payload_error_t payload_get_sequence(
    payload_t *payload, payload_sequence_t *sequence
);

/**
 * @brief 
 */
payload_error_t payload_get_temperature_dc(
    payload_t *payload, payload_temperature_dc_t *temperature_dc
);

/**
 * @brief 
 */
payload_error_t payload_get_version(
    payload_t *payload, payload_version_t *version
);

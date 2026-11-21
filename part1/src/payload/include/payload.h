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
 * @brief Prints the contents of a payload.
 *
 * @param[in] payload Pointer to the payload to print.
 *
 * @note If payload is NULL, nothing is printed.
 */
void payload_print(payload_t *payload);

/**
 * @brief Prints the message corresponding to a payload error.
 *
 * @param[in] error Payload error code.
 *
 * @note If the error code is undefined, nothing is printed.
 */
void payload_print_error(payload_error_t error);

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
 * @brief BLE payload.
 */
typedef struct payload {
    /**
     * @brief Battery voltage in millivolts.
     */
    payload_battery_mv_t battery_mv;

    /**
     * @brief CRC-8 value calculated over the payload data.
     */
    crc8_value_t crc8_value;

    /**
     * @brief Identifies the device type.
     */
    payload_device_type_t device_type;

    /**
     * @brief Device status flags.
     */
    payload_flags_t flags;

    /**
     * @brief Flow rate in liters per hour.
     */
    payload_flow_lph_t flow_lph;

    /**
     * @brief Accumulated pulse count.
     */
    payload_pulse_count_t pulse_count;

    /**
     * @brief Reserved bits in the payload.
     *
     * Reserved bits are expected to be zero.
     */
    payload_reserved_t reserved;

    /**
     * @brief Payload sequence number.
     *
     * The sequence number is an 8-bit value and wraps around after reaching its
     * maximum value.
     */
    payload_sequence_t sequence;

    /**
     * @brief Temperature in deci-degrees Celsius.
     *
     * For example, a value of -57 represents -5.7 degrees Celsius.
     */
    payload_temperature_dc_t temperature_dc;

    /**
     * @brief Payload format version.
     */
    payload_version_t version;
} payload_t;

/**
 * @brief Size of the BLE payload in bytes.
 */
#define PAYLOAD_SIZE_BYTES 12

/**
 * @brief Decodes a byte array into a payload structure.
 *
 * Extracts the payload fields and validates their encoded values.
 *
 * @param[out] payload Pointer to the structure where decoded fields are stored.
 * @param[in] bytes Byte array containing the encoded payload.
 *
 * @return PAYLOAD_NO_ERROR if decoding succeeds. Otherwise, an error indicating
 * the first validation failure.
 *
 * @note The input array must contain at least PAYLOAD_SIZE_BYTES bytes.
 */
payload_error_t payload_decode(
    payload_t *payload, uint8_t bytes[PAYLOAD_SIZE_BYTES]
);

/**
 * @brief Encodes a payload structure into a byte array.
 *
 * Converts the payload fields into their byte representation and
 * generates the corresponding CRC value.
 *
 * @param[in] payload Payload structure to encode.
 * @param[out] bytes Byte array where the encoded payload is stored.
 *
 * @return PAYLOAD_NO_ERROR if encoding succeeds;
 *         otherwise, an error indicating invalid payload data.
 *
 * @note The output array must have space for PAYLOAD_SIZE_BYTES bytes.
 */
payload_error_t payload_encode(
    payload_t payload, uint8_t *bytes[PAYLOAD_SIZE_BYTES]
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

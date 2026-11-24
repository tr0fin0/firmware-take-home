/**
 * @file payload_types.h
 * @brief BLE payload types definitions.
 *
 * @author Guilherme Nunes Trofino
 * @date 2026-11-21
 */

#pragma once

#include <stdint.h>

/**
 * @brief Payload validation and decoding error codes.
 *
 * Enumerates the possible errors detected when encoding or decoding a payload.
 */
typedef enum payload_error {
    /**
     * @brief No error occurred.
     */
    PAYLOAD_NO_ERROR = 0,

    /**
     * @brief Battery voltage exceeds the maximum permitted value.
     */
    PAYLOAD_ERROR_BATTERY_ABOVE_MAXIMAL,

    /**
     * @brief Battery voltage is below the minimum permitted value.
     */
    PAYLOAD_ERROR_BATTERY_BELOW_MINIMAL,

    /**
     * @brief CRC-8 checksum verification failed.
     */
    PAYLOAD_ERROR_CRC8_CHECK_FAIL,

    /**
     * @brief The device type field contains an unsupported value.
     */
    PAYLOAD_ERROR_DEVICE_TYPE_INVALID,

    /**
     * @brief The flow exponent field contains an invalid value.
     */
    PAYLOAD_ERROR_FLOW_EXPONENT_INVALID,

    /**
     * @brief The reserved field contains an invalid value.
     */
    PAYLOAD_ERROR_RESERVED_INVALID,

    /**
     * @brief Temperature exceeds the maximum permitted value.
     */
    PAYLOAD_ERROR_TEMPERATURE_ABOVE_MAXIMAL,

    /**
     * @brief Temperature is below the minimum permitted value.
     */
    PAYLOAD_ERROR_TEMPERATURE_BELOW_MINIMAL,

    /**
     * @brief The payload version field contains an unsupported value.
     */
    PAYLOAD_ERROR_VERSION_INVALID,

    /**
     * @brief Number of payload error codes, excluding PAYLOAD_NO_ERROR.
     */
    NUMBER_OF_PAYLOAD_ERRORS
} payload_error_t;

/**
 * @brief Supported device types encoded in the payload.
 */
typedef enum payload_device_type {
    /**
     * @brief Invalid or unsupported device type.
     */
    PAYLOAD_DEVICE_TYPE_INVALID = -1,

    /**
     * @brief Device type 1, represented by the value 0x12.
     */
    PAYLOAD_DEVICE_TYPE_1 = 0x12,

    /**
     * @brief Number of supported device types.
     */
    NUMBER_OF_PAYLOAD_DEVICE_TYPES = 1
} payload_device_type_t;

/**
 * @brief Payload status flags.
 *
 * Represents the individual status flags that may be set in a payload.
 */
typedef enum payload_flag {
    /**
     * @brief Invalid or unsupported flag.
     */
    PAYLOAD_FLAG_INVALID = -1,

    /**
     * @brief Indicates that the battery voltage is low.
     */
    PAYLOAD_FLAG_BATTERY_LOW,

    /**
     * @brief Indicates that a violation has been detected.
     */
    PAYLOAD_FLAG_VIOLATION,

    /**
     * @brief Indicates that the flow direction is reversed.
     */
    PAYLOAD_FLAG_FLUX_REVERSED,

    /**
     * @brief Number of defined payload flags, excluding the invalid value.
     */
    NUMBER_OF_PAYLOAD_FLAGS
} payload_flag_t;

/**
 * @brief Supported payload format versions.
 */
typedef enum payload_version {
    /**
     * @brief Invalid or unsupported payload version.
     */
    PAYLOAD_VERSION_INVALID = -1,

    /**
     * @brief Payload format version 0.
     */
    PAYLOAD_VERSION_0,

    /**
     * @brief Payload format version 1.
     */
    PAYLOAD_VERSION_1,

    /**
     * @brief Number of supported payload versions.
     */
    NUMBER_OF_PAYLOAD_VERSIONS
} payload_version_t;

/**
 * @brief Decoded payload status flags.
 *
 * Stores the boolean representation of the status flags extracted
 * from a payload.
 */
typedef struct payload_flags {
    /**
     * @brief Indicates whether the battery voltage is low.
     */
    bool battery_low;

    /**
     * @brief Indicates whether a violation has been detected.
     */
    bool violation;

    /**
     * @brief Indicates whether the flow direction is reversed.
     */
    bool flow_reversed;
} payload_flags_t;

/**
 * @brief Battery voltage in millivolts (mV).
 */
typedef uint16_t payload_battery_mv_t;

/**
 * @brief Volumetric flow rate in liters per hour (L/h).
 */
typedef float payload_flow_lph_t;

/**
 * @brief Accumulated pulse count.
 */
typedef uint32_t payload_pulse_count_t;

/**
 * @brief Reserved payload field.
 *
 * Stores the value of a reserved field in the payload format.
 */
typedef uint8_t payload_reserved_t;

/**
 * @brief Payload sequence number.
 *
 * Identifies the sequence of a payload transmission.
 */
typedef uint8_t payload_sequence_t;

/**
 * @brief Temperature in deci-degrees Celsius (0.1 °C).
 *
 * A value of 253 represents 25.3 °C.
 */
typedef int16_t payload_temperature_dc_t;

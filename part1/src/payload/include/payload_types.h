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
 * @brief
 */
typedef enum payload_error {
    /**
     * @brief
     */
    PAYLOAD_NO_ERROR = 0,

    /**
     * @brief
     */
    PAYLOAD_ERROR_BATTERY_ABOVE_MAXIMAL,

    /**
     * @brief
     */
    PAYLOAD_ERROR_BATTERY_BELOW_MINIMAL,

    /**
     * @brief
     */
    PAYLOAD_ERROR_CRC8_CHECK_FAIL,

    /**
     * @brief
     */
    PAYLOAD_ERROR_DEVICE_TYPE_INVALID,

    /**
     * @brief
     */
    PAYLOAD_ERROR_FLAG_INVALID,

    /**
     * @brief
     */
    PAYLOAD_ERROR_FLOW_EXPONENT_INVALID,

    /**
     * @brief
     */
    PAYLOAD_ERROR_RESERVED_INVALID,

    /**
     * @brief
     */
    PAYLOAD_ERROR_TEMPERATURE_ABOVE_MAXIMAL,

    /**
     * @brief
     */
    PAYLOAD_ERROR_TEMPERATURE_BELOW_MINIMAL,

    /**
     * @brief
     */
    PAYLOAD_ERROR_VERSION_INVALID,
} payload_error_t;

/**
 * @brief
 */
typedef enum payload_device_type {
    /**
     * @brief
     */
    PAYLOAD_DEVICE_TYPE_INVALID = -1,

    /**
     * @brief
     */
    PAYLOAD_DEVICE_TYPE_1 = 0x12,

    /**
     * @brief
     */
    NUMBER_OF_PAYLOAD_DEVICE_TYPES = 1
} payload_device_type_t;

/**
 * @brief
 */
typedef enum payload_flag {
    /**
     * @brief
     */
    PAYLOAD_FLAG_INVALID = -1,

    /**
     * @brief
     */
    PAYLOAD_FLAG_BATTERY_LOW,

    /**
     * @brief
     */
    PAYLOAD_FLAG_VIOLATION,

    /**
     * @brief
     */
    PAYLOAD_FLAG_FLUX_REVERSED,

    /**
     * @brief
     */
    NUMBER_OF_PAYLOAD_FLAGS
} payload_flag_t;

/**
 * @brief 
 */
typedef enum payload_version {
    /**
     * @brief
     */
    PAYLOAD_VERSION_INVALID = -1,

    /**
     * @brief
     */
    PAYLOAD_VERSION_0,

    /**
     * @brief
     */
    PAYLOAD_VERSION_1,

    /**
     * @brief
     */
    NUMBER_OF_PAYLOAD_VERSIONS
} payload_version_t;

/**
 * @brief
 */
typedef struct payload_flags {
    /**
     * @brief
     */
    bool battery_low;

    /**
     * @brief
     */
    bool violation;

    /**
     * @brief
     */
    bool flow_reversed;
} payload_flags_t;

/**
 * @brief 
 */
typedef uint16_t payload_battery_mv_t;

/**
 * @brief Flow in liters per hour.
 */
typedef float payload_flow_lph_t;

/**
 * @brief 
 */
typedef uint32_t payload_pulse_count_t;

/**
 * @brief 
 */
typedef uint8_t payload_reserved_t;

/**
 * @brief 
 */
typedef uint8_t payload_sequence_t;

/**
 * @brief 
 */
typedef int16_t payload_temperature_dc_t;

/**
 * @file registry.h
 * @brief Registry types defintions for testing results.
 *
 * @author Guilherme Nunes Trofino
 * @date 2026-11-22
 */

#pragma once

#include "registry_constants.h"
#include <stdint.h>

/**
 * @brief Registry failure code represented by an enumeration.
 *
 * The value is serialized as one byte. Zero indicates that the board passed
 * the test. Values from 0x01 through 0xFF are reserved for application-
 * specific failure codes.
 */
typedef enum registry_error_code {
    /**
     * @brief Indicates that the board passed the test.
     */
    REGISTRY_NO_ERROR = (uint8_t) 0x00,

    /**
     * @brief Largest value reserved for an application-specific error code.
     */
    REGISTRY_MAX_ERROR_CODE = (uint8_t) 0xFF,

    /**
     * @brief One past the largest one-byte error-code value.
     *
     * Its numeric value is 256, counting all 256 byte values including
     * `REGISTRY_NO_ERROR`.
     */
    NUMBER_OF_REGISTRY_ERRORS
} registry_error_code_t;

/**
 * @brief Registry character represented by a byte.
 */
typedef uint8_t registry_character_t;

/**
 * @brief Registry ISO 8601 timestamp.
 */
typedef uint32_t registry_timestamp_t;

/**
 * @brief Registry produced by testing one board.
 */
typedef struct registry {
    /**
     * @brief Test result fixed-width not null-terminate serial number.
     *
     * Every element must contain an ASCII alphanumeric character: `0`-`9`,
     * `A`-`Z`, or `a`-`z`.
     */
    registry_character_t serial_number[REGISTRY_SERIAL_NUMBER_SIZE];

    /**
     * @brief Test result error code.
     * 
     * Expected @ref REGISTRY_NO_ERROR for approval.
     */
    registry_error_code_t error_code;

    /**
     * @brief Test result timestamp.
     */
    registry_timestamp_t timestamp;
} registry_t;

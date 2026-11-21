/**
 * @file crc8.h
 * @brief Cyclic Redundancy Check
 *
 * @author Guilherme Nunes Trofino
 * @date 2026-11-21
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief CRC-8 initial register value.
 *
 * Defines the initial value loaded into the CRC register before processing the
 * input data.
 */
typedef uint8_t crc8_init_value_t;

/**
 * @brief CRC-8 generator polynomial.
 *
 * Defines the polynomial used for CRC computation, represented as an 8-bit
 * value.
 */
typedef uint8_t crc8_polynomial_t;

/**
 * @brief CRC-8 input reflection configuration.
 *
 * Determines whether each input byte is processed with its bits reflected,
 * least significant bit first.
 */
typedef bool crc8_reflection_in_t;

/**
 * @brief CRC-8 output reflection configuration.
 *
 * Determines whether the final CRC value is reflected before applying the final
 * XOR operation.
 */
typedef bool crc8_reflection_out_t;

/**
 * @brief CRC-8 computed value.
 *
 * Represents the resulting 8-bit cyclic redundancy check.
 */
typedef uint8_t crc8_value_t;

/**
 * @brief CRC-8 final XOR value.
 *
 * Defines the value XORed with the CRC register after processing the input data
 * and applying output reflection.
 */
typedef uint8_t crc8_xor_out_t;

/**
 * @brief CRC-8 algorithm configuration.
 *
 * Groups the parameters required to configure a CRC-8 computation.
 */
typedef struct crc8_config {
    /**
     * @brief Initial CRC register value.
     */
    crc8_init_value_t init_value;

    /**
     * @brief Generator polynomial.
     */
    crc8_polynomial_t polynomial;

    /**
     * @brief Enables or disables input bit reflection.
     */
    crc8_reflection_in_t reflection_in;

    /**
     * @brief Enables or disables output bit reflection.
     */
    crc8_reflection_out_t reflection_out;

    /**
     * @brief Final XOR value.
     */
    crc8_xor_out_t xor_out;
} crc8_config_t;

/**
 * @brief Computes the CRC-8 checksum of an input byte sequence.
 *
 * Computes a CRC-8 checksum using the algorithm parameters specified in the
 * configuration structure.
 *
 * @param[in] config CRC-8 algorithm configuration.
 * @param[in] bytes Pointer to the input byte sequence.
 * @param[in] num_bytes Number of bytes to process.
 *
 * @return Computed CRC-8 checksum.
 */
crc8_value_t crc8_compute(
    crc8_config_t config, const uint8_t *bytes, uint8_t num_bytes
);

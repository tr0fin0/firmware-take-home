/**
 * @file test.h
 * @brief Payload testing functions.
 *
 * @author Guilherme Nunes Trofino
 * @date 2026-11-22
 */

#pragma once

/**
 * @brief Tests successful payload decoding and encoding.
 */
void test_decode_encode(void);

/**
 * @brief Tests payload decoding with an invalid CRC8 checksum.
 */
void test_error_decode_crc8(void);

/**
 * @brief Tests payload decoding with an invalid version.
 */
void test_error_decode_version(void);

/**
 * @brief Tests payload encoding with an invalid version.
 */
void test_error_encode_version(void);

/**
 * @brief Tests payload decoding with an invalid device type.
 */
void test_error_decode_device_type(void);

/**
 * @brief Tests payload encoding with an invalid device type.
 */
void test_error_encode_device_type(void);

/**
 * @brief Tests payload decoding with battery voltage above the maximum.
 */
void test_error_decode_battery_above_maximal(void);

/**
 * @brief Tests payload encoding with battery voltage above the maximum.
 */
void test_error_encode_battery_above_maximal(void);

/**
 * @brief Tests payload decoding with battery voltage below the minimum.
 */
void test_error_decode_battery_below_minimal(void);

/**
 * @brief Tests payload encoding with battery voltage below the minimum.
 */
void test_error_encode_battery_below_minimal(void);

/**
 * @brief Tests payload decoding with temperature above the maximum.
 */
void test_error_decode_temperature_above_maximal(void);

/**
 * @brief Tests payload encoding with temperature above the maximum.
 */
void test_error_encode_temperature_above_maximal(void);

/**
 * @brief Tests payload decoding with temperature below the minimum.
 */
void test_error_decode_temperature_below_minimal(void);

/**
 * @brief Tests payload encoding with temperature below the minimum.
 */
void test_error_encode_temperature_below_minimal(void);

/**
 * @brief Tests payload decoding with an invalid flow exponent.
 */
void test_error_decode_flow_exponent_invalid(void);

/**
 * @brief Tests payload encoding with an unrepresentable flow value.
 */
void test_error_encode_flow_exponent_invalid(void);

/**
 * @brief Tests payload decoding with invalid reserved bits.
 */
void test_error_decode_reserved(void);

/**
 * @brief Tests payload encoding with invalid reserved bits.
 */
void test_error_encode_reserved(void);

/**
 * @brief Tests encoding and decoding of all flag combinations.
 */
void test_flags_combinations(void);

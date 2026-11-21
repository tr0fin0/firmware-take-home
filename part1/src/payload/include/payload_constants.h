/**
 * @file payload_constants.h
 * @brief BLE payload constants definitions.
 *
 * @author Guilherme Nunes Trofino
 * @date 2026-11-21
 */

#pragma once

#include "payload_types.h"

/**
 * @brief Maximum accepted battery charge in mili-volts.
 */
#define PAYLOAD_BATTERY_MAXIMUM_MV ((payload_battery_mv_t) 3600)

/**
 * @brief Minimum accepted battery charge in mili-volts.
 */
#define PAYLOAD_BATTERY_MINIMUM_MV ((payload_battery_mv_t) 1800)

/**
 * @brief Maximum accepted temperature in deci-celsius.
 */
#define PAYLOAD_TEMPERATURE_MAXIMUM_DC ((payload_temperature_dc_t) +850)

/**
 * @brief Minimum accepted temperature in deci-celsius.
 */
#define PAYLOAD_TEMPERATURE_MINIMUM_DC ((payload_temperature_dc_t) -400)

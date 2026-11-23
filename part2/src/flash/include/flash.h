/**
 * @file flash.h
 * @brief Configurable RAM-backed flash abstraction with failure injection.
 *
 * @author Guilherme Nunes Trofino
 * @date 2026-11-22 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <stdbool.h>
#include <stdint.h>

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Flash functions errors.
 */
typedef enum flash_error {
    /**
     * @brief Function completed successfully without errors.
     */
    FLASH_NO_ERROR = 0,

    /**
     * @brief Argument pointer is null or is otherwise invalid.
     */
    FLASH_ERROR_INVALID_ARGUMENT,

    /**
     * @brief Flash geometry or backing storage configuration is invalid.
     */
    FLASH_ERROR_INVALID_CONFIGURATION,

    /**
     * @brief The requested address range is outside the configured region.
     */
    FLASH_ERROR_OUT_OF_RANGE,

    /**
     * @brief The address or length does not satisfy the operation alignment.
     */
    FLASH_ERROR_ALIGNMENT,

    /**
     * @brief Programming would require changing a bit from zero to one.
     */
    FLASH_ERROR_REQUIRES_ERASE,

    /**
     * @brief The operation was interrupted by configured failure injection.
     */
    FLASH_ERROR_INJECTED_FAILURE
} flash_error_t;

/**
 * @brief Flash operation used to select a failure-injection target.
 */
typedef enum flash_failure {
    /**
     * @brief Inject a failure during a program operation.
     */
    FLASH_FAILURE_PROGRAM = 1,

    /**
     * @brief Inject a failure during an erase operation.
     */
    FLASH_FAILURE_ERASE = 2
} flash_failure_t;

/**
 * @brief Flash address measured in bytes from the beginning of the region.
 */
typedef uint32_t flash_address_t;

/**
 * @brief Byte stored in the flash memory.
 */
typedef uint8_t flash_byte_t;

/**
 * @brief Number of bytes in one erase block.
 */
typedef uint32_t flash_erase_size_t;

/**
 * @brief Number of bytes in one programming unit.
 */
typedef uint32_t flash_program_size_t;

/**
 * @brief Flash size measured in bytes.
 */
typedef uint32_t flash_size_t;

/**
 * @brief Number of successfully processed units before an injected failure.
 */
typedef uint32_t flash_units_before_failure_t;

/**
 * @brief Configuration of the flash region geometry.
 */
typedef struct flash_config {
    /**
     * @brief Total usable capacity of the flash region, in bytes.
     */
    flash_size_t capacity_bytes;

    /**
     * @brief Erase-block size, in bytes.
     */
    flash_erase_size_t erase_size_bytes;

    /**
     * @brief Minimum programming-unit size, in bytes.
     */
    flash_program_size_t program_size_bytes;
} flash_config_t;

/**
 * @brief RAM-backed flash device.
 *
 * The backing memory is supplied by the caller.
 */
typedef struct flash {
    /**
     * @brief Flash configuration geometry.
     */
    flash_config_t config;

    /**
     * @brief Caller-owned byte array representing persistent flash contents.
     */
    flash_byte_t *storage;

    /**
     * @brief Number of bytes available in the caller-owned storage array.
     */
    flash_size_t storage_size_bytes;

    /**
     * @brief Indicates whether the flash handle has been initialized.
     */
    bool initialized;

    /**
     * @brief Indicates whether a one-shot failure injection is armed.
     */
    bool failure_injected;

    /**
     * @brief Operation targeted by the current failure injection.
     */
    flash_failure_t failure;

    /**
     * @brief Matching operation units allowed before failure is injected.
     */
    flash_units_before_failure_t failure_units_remaining;
} flash_t;

/**
 * @brief Maximum program-unit size supported by the flash implementation.
 */
#define FLASH_MAX_PROGRAM_UNIT_SIZE_BYTES 256U

/**
 * @brief Attach a flash handle to caller-owned backing memory.
 *
 * This function validates and stores the geometry but does not erase or
 * otherwise modify the backing memory. Call `flash_format` explicitly when a
 * newly provisioned, erased region is required. Do not call `flash_format`
 * during reboot simulation.
 *
 * @param flash Flash handle to initialize.
 * @param config Flash configuration geometry.
 * @param storage Caller-owned byte array that represents flash contents.
 * @param storage_size_bytes Size of the supplied backing array.
 *
 * @return `FLASH_NO_ERROR` on success; otherwise, an error status.
 */
flash_error_t flash_init(
    flash_t *flash,
    const flash_config_t *config,
    flash_byte_t *storage,
    flash_size_t storage_size_bytes
);

/**
 * @brief Erase the complete configured flash region.
 *
 * It is subject to erase alignment and failure injection.
 *
 * @param flash Initialized flash handle.
 *
 * @return `FLASH_NO_ERROR` on success; otherwise, an error status.
 */
flash_error_t flash_format(flash_t *flash);

/**
 * @brief Read bytes from the configured flash region.
 *
 * Reads may use any byte address and length, programming and erasing have
 * stricter alignment requirements.
 *
 * @param flash Initialized flash handle.
 * @param address First byte address to read.
 * @param destination Buffer receiving the read bytes.
 * @param size_bytes Number of bytes to read. A zero-length read is accepted.
 *
 * @return `FLASH_NO_ERROR` on success; otherwise, an error status.
 */
flash_error_t flash_read(
    const flash_t *flash,
    flash_address_t address,
    flash_byte_t *destination,
    flash_size_t size_bytes
);

/**
 * @brief Program bytes into flash.
 *
 * Programming requires an address and length aligned to the configured
 * programming unit. Bits may only transition from one to zero. The operation
 * can be interrupted after a configured number of complete programming units.
 *
 * @param flash Initialized flash handle.
 * @param address First byte address to program.
 * @param source Bytes to program.
 * @param size_bytes Number of bytes to program.
 *
 * @note A zero-length program is accepted when the address is within the flash
 * region.
 *
 * @return `FLASH_NO_ERROR` on success; otherwise, an error status.
 */
flash_error_t flash_program(
    flash_t *flash,
    flash_address_t address,
    const flash_byte_t *source,
    flash_size_t size_bytes
);

/**
 * @brief Erase one or more complete erase blocks.
 *
 * The size must be nonzero, and both the address and size must be aligned to
 * the configured erase-block size. The operation can be interrupted after a
 * configured number of complete erase blocks.
 *
 * @param flash Initialized flash handle.
 * @param address First byte address to erase.
 * @param size_bytes Number of bytes to erase.
 *
 * @return `FLASH_NO_ERROR` on success; otherwise, an error status.
 */
flash_error_t flash_erase(
    flash_t *flash, flash_address_t address, flash_size_t size_bytes
);

/**
 * @brief Arm a one-shot failure after a number of matching operation units.
 *
 * For programming, one unit is the configured programming unit. For erasing,
 * one unit is one erase block. The count is cumulative across matching calls:
 * after the specified number of units have completed, the next matching unit
 * fails. A value of zero fails before the first matching unit is modified.
 * The injection disables itself when triggered. If an operation ends before
 * the failure point, the injection remains armed for the next matching call.
 *
 * @param flash Initialized flash handle.
 * @param failure Operation to interrupt.
 * @param units_before_failure Number of complete matching units allowed.
 *
 * @return `FLASH_NO_ERROR` on success; otherwise, an error status.
 */
flash_error_t flash_inject_failure(
    flash_t *flash,
    flash_failure_t failure,
    flash_units_before_failure_t units_before_failure
);

/**
 * @brief Disarm any pending failure injection.
 *
 * @param flash Initialized flash handle.
 *
 * @return `FLASH_NO_ERROR` on success; otherwise, an error status.
 */
flash_error_t flash_clear_failure_injection(flash_t *flash);

/**
 * @brief Retrieve the active flash geometry.
 *
 * @param flash Initialized flash handle.
 * @param config Destination for the geometry.
 *
 * @return `FLASH_NO_ERROR` on success; otherwise, an error status.
 */
flash_error_t flash_get_config(const flash_t *flash, flash_config_t *config);

#include "flash.h"

#include <string.h>

static bool flash_is_failure(flash_t *flash, flash_failure_t failure) {
    if (
        !(flash->failure_injected) ||
         (flash->failure != failure)
    ) {
        return false;
    }

    if (flash->failure_units_remaining == 0U) {
        flash->failure_injected = false;

        return true;
    }

    return false;
}

static bool flash_is_initialized(const flash_t *flash) {
    return (
        (flash != NULL) &&
        (flash->initialized) &&
        (flash->storage != NULL)
    );
}

static bool flash_is_range_valid(
    const flash_t *flash,
    flash_address_t address,
    flash_size_t size_bytes
) {
    if (!flash_is_initialized(flash)) {
        return false;
    }

    if (address > flash->config.capacity_bytes) {
        return false;
    }

    return size_bytes <= (flash->config.capacity_bytes - address);
}

static void flash_record_successful_unit(
    flash_t *flash, flash_failure_t failure
) {
    if (
        (flash->failure_injected) &&
        (flash->failure == failure) &&
        (flash->failure_units_remaining > 0U)
    ) {
        --flash->failure_units_remaining;
    }
}

flash_error_t flash_init(
    flash_t *flash,
    const flash_config_t *config,
    flash_byte_t *storage,
    flash_size_t storage_size_bytes
) {
    if (flash == NULL) {
        return FLASH_ERROR_INVALID_ARGUMENT;
    }

    memset(flash, 0, sizeof(*flash));

    if ((config == NULL) || (storage == NULL)) {
        return FLASH_ERROR_INVALID_ARGUMENT;
    }

    if (
        (config->capacity_bytes == 0U) ||
        (config->erase_size_bytes == 0U) ||
        (config->program_size_bytes == 0U) ||
        (config->program_size_bytes > FLASH_MAX_PROGRAM_UNIT_SIZE_BYTES) ||
        (config->capacity_bytes > storage_size_bytes) ||
        ((config->capacity_bytes % config->erase_size_bytes) != 0U) ||
        ((config->erase_size_bytes % config->program_size_bytes) != 0U)
    ) {
        return FLASH_ERROR_INVALID_CONFIGURATION;
    }

    flash->config = *config;
    flash->storage = storage;
    flash->storage_size_bytes = storage_size_bytes;
    flash->initialized = true;

    return FLASH_NO_ERROR;
}

flash_error_t flash_format(flash_t *flash) {
    if (!flash_is_initialized(flash)) {
        return FLASH_ERROR_INVALID_ARGUMENT;
    }

    return flash_erase(flash, 0U, flash->config.capacity_bytes);
}

flash_error_t flash_read(
    const flash_t *flash,
    flash_address_t address,
    flash_byte_t *destination,
    flash_size_t size_bytes
) {
    if (!flash_is_initialized(flash)) {
        return FLASH_ERROR_INVALID_ARGUMENT;
    }

    if ((destination == NULL) && (size_bytes > 0U)) {
        return FLASH_ERROR_INVALID_ARGUMENT;
    }

    if (!flash_is_range_valid(flash, address, size_bytes)) {
        return FLASH_ERROR_OUT_OF_RANGE;
    }

    if (size_bytes > 0U) {
        memcpy(destination, &flash->storage[address], size_bytes);
    }

    return FLASH_NO_ERROR;
}

flash_error_t flash_program(
    flash_t *flash,
    flash_address_t address,
    const flash_byte_t *source,
    flash_size_t size_bytes
) {
    if (!flash_is_initialized(flash)) {
        return FLASH_ERROR_INVALID_ARGUMENT;
    }

    if ((source == NULL) && (size_bytes > 0U)) {
        return FLASH_ERROR_INVALID_ARGUMENT;
    }

    if (!flash_is_range_valid(flash, address, size_bytes)) {
        return FLASH_ERROR_OUT_OF_RANGE;
    }

    if (size_bytes == 0U) {
        return FLASH_NO_ERROR;
    }

    if ((address % flash->config.program_size_bytes) != 0U) {
        return FLASH_ERROR_ALIGNMENT;
    }

    if ((size_bytes % flash->config.program_size_bytes) != 0U) {
        return FLASH_ERROR_ALIGNMENT;
    }

    for (flash_size_t index = 0U; index < size_bytes; ++index) {
        const flash_byte_t current      = flash->storage[address + index];
        const flash_byte_t requested    = source[index];

        if ((current & requested) != requested) {
            return FLASH_ERROR_REQUIRES_ERASE;
        }
    }

    for (
        flash_size_t offset = 0U;
        offset < size_bytes;
        offset += flash->config.program_size_bytes
    ) {
        if (flash_is_failure(flash, FLASH_FAILURE_PROGRAM)) {
            return FLASH_ERROR_INJECTED_FAILURE;
        }

        for (
            flash_program_size_t index = 0U;
            index < flash->config.program_size_bytes;
            index++
        ) {
            const flash_size_t byte_offset          = offset + index;
            flash->storage[address + byte_offset]  &= source[byte_offset];
        }

        flash_record_successful_unit(flash, FLASH_FAILURE_PROGRAM);
    }

    return FLASH_NO_ERROR;
}

flash_error_t flash_erase(
    flash_t *flash, flash_address_t address, flash_size_t size_bytes
) {
    if (!flash_is_initialized(flash)) {
        return FLASH_ERROR_INVALID_ARGUMENT;
    }

    if (size_bytes == 0U) {
        return FLASH_ERROR_INVALID_ARGUMENT;
    }

    if (!flash_is_range_valid(flash, address, size_bytes)) {
        return FLASH_ERROR_OUT_OF_RANGE;
    }

    if ((address % flash->config.erase_size_bytes) != 0U) {
        return FLASH_ERROR_ALIGNMENT;
    }

    if ((size_bytes % flash->config.erase_size_bytes) != 0U) {
        return FLASH_ERROR_ALIGNMENT;
    }

    for (
        flash_size_t offset = 0U;
        offset < size_bytes;
        offset += flash->config.erase_size_bytes
    ) {
        if (flash_is_failure(flash, FLASH_FAILURE_ERASE)) {
            return FLASH_ERROR_INJECTED_FAILURE;
        }

        memset(
            &flash->storage[address + offset],
            0xFF,
            flash->config.erase_size_bytes
        );

        flash_record_successful_unit(flash, FLASH_FAILURE_ERASE);
    }

    return FLASH_NO_ERROR;
}

flash_error_t flash_inject_failure(
    flash_t *flash,
    flash_failure_t operation,
    flash_units_before_failure_t units_before_failure
) {
    if (!flash_is_initialized(flash)) {
        return FLASH_ERROR_INVALID_ARGUMENT;
    }

    if (
        (operation != FLASH_FAILURE_PROGRAM) &&
        (operation != FLASH_FAILURE_ERASE)
    ) {
        return FLASH_ERROR_INVALID_ARGUMENT;
    }

    flash->failure_injected         = true;
    flash->failure                  = operation;
    flash->failure_units_remaining  = units_before_failure;

    return FLASH_NO_ERROR;
}

flash_error_t flash_clear_failure_injection(flash_t *flash) {
    if (!flash_is_initialized(flash)) {
        return FLASH_ERROR_INVALID_ARGUMENT;
    }

    flash->failure_injected         = false;
    flash->failure_units_remaining  = 0U;

    return FLASH_NO_ERROR;
}

flash_error_t flash_get_config(const flash_t *flash, flash_config_t *config) {
    if (!flash_is_initialized(flash) || (config == NULL)) {
        return FLASH_ERROR_INVALID_ARGUMENT;
    }

    *config = flash->config;

    return FLASH_NO_ERROR;
}

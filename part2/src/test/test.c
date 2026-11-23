#include <assert.h>
#include "flash.h"
#include "queue.h"
#include "registry.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "test.h"

/**
 * @brief Total capacity of the fake flash used by the test fixture, in bytes.
 */
#define TEST_FLASH_CAPACITY_BYTES           4096U

/**
 * @brief Erase block size of the fake flash used by the test fixture, in bytes.
 */
#define TEST_FLASH_ERASE_BLOCK_SIZE_BYTES    256U

/**
 * @brief Programming unit size of the fake flash used by the test fixture,
 * in bytes.
 */
#define TEST_FLASH_PROGRAM_UNIT_SIZE_BYTES     4U

/**
 * @brief Test fixture containing persistent storage and volatile driver state.
 *
 * Holds the configuration, backing memory and driver instances required to
 * initialize and operate the registry queue during host-side tests.
 */
typedef struct test {
    /**
     * @brief Configuration describing the fake flash memory geometry.
     */
    flash_config_t config;

    /**
     * @brief RAM-backed storage representing the contents of flash memory.
     *
     * Preserves its contents when the flash and queue instances are
     * reinitialized to simulate a system reboot.
     */
    flash_byte_t storage[TEST_FLASH_CAPACITY_BYTES];

    /**
     * @brief Fake flash driver instance operating on the backing storage.
     */
    flash_t flash;

    /**
     * @brief Queue instance under test.
     *
     * Holds the volatile representation of the persistent registry queue.
     */
    queue_t queue;
} test_t;

/**
 * @brief Shared test fixture.
 *
 * Provides the flash storage and driver instances used by the test functions.
 */
static test_t test;

/**
 * @brief Default flash configuration used by the test fixture.
 *
 * Defines the capacity, erase block size and programming unit size of the
 * fake flash memory.
 */
static const flash_config_t test_flash_config = {
    .capacity_bytes     = TEST_FLASH_CAPACITY_BYTES,
    .erase_size_bytes   = TEST_FLASH_ERASE_BLOCK_SIZE_BYTES,
    .program_size_bytes = TEST_FLASH_PROGRAM_UNIT_SIZE_BYTES
};

/**
 * @brief Initializes the test fixture and an empty registry queue.
 *
 * Copies the provided flash configuration, initializes the backing storage
 * to the erased state and initializes the fake flash and queue instances.
 *
 * @param[in] config Flash configuration to apply to the test fixture.
 *
 * @retval true The flash and queue were successfully initialized.
 * @retval false The configuration is invalid or an initialization failed.
 */
static bool test_initialize(const flash_config_t *config) {
    if (config == NULL) {
        return false;
    }

    test.config = *config;
    memset(test.storage, 0xFF, sizeof(test.storage));

    if (
        flash_init(
            &test.flash,
            &test.config,
            test.storage,
            (flash_size_t) sizeof(test.storage)
        ) != FLASH_NO_ERROR
    ) {
        return false;
    }

    return (
        queue_init(&test.queue, &test.flash) == QUEUE_NO_ERROR
    );
}

/**
 * @brief Simulates a system reboot while preserving flash contents.
 *
 * Reinitializes the fake flash driver and queue instances using the existing
 * test configuration and backing storage.
 *
 * The queue initialization reconstructs its volatile state from the previously
 * persisted snapshot.
 *
 * @retval true The flash and queue were successfully reinitialized.
 * @retval false A flash or queue initialization operation failed.
 *
 * @note The backing storage is not erased or modified before reinitialization.
 */
static bool test_reboot(void) {
    if (
        flash_init(
            &test.flash,
            &test.config,
            test.storage,
            (flash_size_t) sizeof(test.storage)
        ) != FLASH_NO_ERROR
    ) {
        return false;
    }

    return (
        queue_init(&test.queue, &test.flash) == QUEUE_NO_ERROR
    );
}

/**
 * @brief Creates a registry with a deterministic serial number and test data.
 *
 * Generates a fixed-width serial number using the `BOARD` prefix followed
 * by the decimal representation of the supplied identifier, aligned to the
 * right within the remaining serial number positions.
 *
 * Assigns the provided error code and timestamp to the registry.
 *
 * @param[in] identifier Numeric identifier used to generate the serial number.
 * @param[in] error_code Test result error code.
 * @param[in] timestamp Registry timestamp.
 *
 * @return Initialized registry structure.
 *
 * @note The registry is zero-initialized before its fields are populated.
 */
static registry_t test_registry_create(
    uint32_t identifier,
    registry_error_code_t error_code,
    registry_timestamp_t timestamp
) {
    registry_t registry = {0};

    registry.serial_number[0] = (registry_character_t)'B';
    registry.serial_number[1] = (registry_character_t)'O';
    registry.serial_number[2] = (registry_character_t)'A';
    registry.serial_number[3] = (registry_character_t)'R';
    registry.serial_number[4] = (registry_character_t)'D';

    for (uint32_t i = REGISTRY_SERIAL_NUMBER_SIZE; i > 5U; i--) {
        registry.serial_number[i - 1U] = (
            (registry_character_t) ('0' + (identifier % 10U))
        );

        identifier /= 10U;
    }

    registry.error_code = error_code;
    registry.timestamp  = timestamp;

    return registry;
}

/**
 * @brief Compares two registries for equality.
 *
 * Compares the complete fixed-width serial numbers, error codes and
 * timestamps of both registries.
 *
 * @param[in] l Pointer to the first registry.
 * @param[in] r Pointer to the second registry.
 *
 * @retval true All registry fields are equal.
 * @retval false At least one registry field differs.
 */
static bool test_registry_equal(const registry_t *l, const registry_t *r) {
    return (
        (
            memcmp(
                l->serial_number,
                r->serial_number,
                REGISTRY_SERIAL_NUMBER_SIZE
            ) == 0
        ) &&
        (l->error_code  == r->error_code) &&
        (l->timestamp   == r->timestamp)
    );
}

/**
 * @brief Verifies that the queue contains the expected registries in FIFO order.
 *
 * Repeatedly retrieves the oldest registry using @ref queue_peek, compares
 * it against the corresponding expected registry and removes it using
 * @ref queue_ack.
 *
 * After processing all expected registries, verifies that the queue is empty.
 *
 * @param[in] expected_records Array containing the expected registries
 *                             in FIFO order.
 * @param[in] expected_count Number of expected registries.
 *
 * @retval true All registries were retrieved in the expected order and
 * the queue is empty afterward.
 *
 * @retval false A registry differs, a queue operation fails or unexpected
 * registries remain in the queue.
 *
 * @note This function modifies the queue by acknowledging every registry
 * successfully retrieved during verification.
 */
static bool test_queue_contains_sequence(
    const registry_t *expected_records, queue_record_count_t expected_count
) {
    for (queue_record_count_t i = 0U; i < expected_count; i++) {
        registry_t actual_record;

        if (queue_peek(&test.queue, &actual_record) != QUEUE_NO_ERROR) {
            return false;
        }

        if (!test_registry_equal(&actual_record, &expected_records[i])) {
            return false;
        }

        if (queue_ack(&test.queue) != QUEUE_NO_ERROR) {
            return false;
        }
    }

    return (
        queue_peek(
            &test.queue, &(registry_t){0}
        ) == QUEUE_ERROR_EMPTY
    );
}

/**
 * @brief Verifies that the queue RAM state matches an expected sequence.
 *
 * Compares the queue record count and each occupied registry against the
 * expected registries without modifying the queue.
 *
 * @param[in] expected_records Array containing the expected registries
 *                             in FIFO order.
 * @param[in] expected_count Expected number of registries in the queue.
 *
 * @retval true The queue contains exactly the expected registries in the
 *              expected FIFO order.
 * @retval false The record count or at least one registry differs.
 *
 * @note This function inspects the queue's internal RAM representation
 * directly and does not access flash.
 */
static bool test_queue_equals(
    const registry_t *expected_records, queue_record_count_t expected_count
) {
    if (test.queue.record_count != expected_count) {
        return false;
    }

    for (queue_record_count_t i = 0U; i < expected_count; i++) {
        if (
            !test_registry_equal(
                &test.queue.records[i],
                &expected_records[i]
            )
        ) {
            return false;
        }
    }

    return true;
}

void test_flash_failure_injection(void) {
    flash_t flash;
    flash_byte_t storage[64];
    flash_byte_t program_data[8] = {
        0x11U, 0x22U, 0x33U, 0x44U, 0x55U, 0x66U, 0x77U, 0x88U
    };
    const flash_config_t config = {
        .capacity_bytes     = 64U,
        .erase_size_bytes   = 16U,
        .program_size_bytes = 4U
    };

    memset(storage, 0xFF, sizeof(storage));
    assert(
        flash_init(
            &flash,
            &config,
            storage,
            sizeof(storage)
        ) == FLASH_NO_ERROR
    );
    assert(
        flash_inject_failure(
            &flash,
            FLASH_FAILURE_PROGRAM,
            1U
        ) == FLASH_NO_ERROR
    );
    assert(
        flash_program(
            &flash,
            0U,
            program_data,
            sizeof(program_data)
        ) == FLASH_ERROR_INJECTED_FAILURE
    );
    assert(memcmp(storage, program_data, 4U) == 0);
    assert(storage[4] == 0xFFU);

    const flash_byte_t second_block_data[4] = {0U, 0U, 0U, 0U};
    assert(
        flash_program(
            &flash,
            16U,
            second_block_data,
            sizeof(second_block_data)
        ) == FLASH_NO_ERROR
    );

    assert(
        flash_inject_failure(
            &flash,
            FLASH_FAILURE_ERASE,
            1U
        ) == FLASH_NO_ERROR
    );
    assert(flash_erase(&flash, 0U, 32U) == FLASH_ERROR_INJECTED_FAILURE);
    assert(storage[0]   == 0xFFU);
    assert(storage[16]  == 0x00U);
    assert(storage[32]  == 0xFFU);

    (void) printf("[success] %s()\n", __func__);
}

void test_flash_initialization_errors(void) {
    flash_t flash;
    flash_byte_t storage[64];
    const flash_config_t flash_config_invalid = {
        .capacity_bytes     = 64U,
        .erase_size_bytes   = 16U,
        .program_size_bytes = 3U
    };

    assert(
        flash_init(
            NULL,
            &flash_config_invalid,
            storage,
            sizeof(storage)
        ) == FLASH_ERROR_INVALID_ARGUMENT
    );
    assert(
        flash_init(
            &flash,
            NULL,
            storage,
            sizeof(storage)
        ) == FLASH_ERROR_INVALID_ARGUMENT
    );
    assert(
        flash_init(
            &flash,
            &flash_config_invalid,
            storage,
            sizeof(storage)
        ) == FLASH_ERROR_INVALID_CONFIGURATION
    );

    const flash_config_t flash_config_valid = {
        .capacity_bytes     = 64U,
        .erase_size_bytes   = 16U,
        .program_size_bytes = 4U
    };

    memset(storage, 0xFF, sizeof(storage));
    assert(
        flash_init(
            &flash,
            &flash_config_valid,
            storage, sizeof(storage)
        ) == FLASH_NO_ERROR
    );
    assert(
        flash_read(
            &flash,
            64U,
            storage,
            1U
        ) == FLASH_ERROR_OUT_OF_RANGE
    );
    assert(
        flash_program(
            &flash,
            1U,
            storage,
            4U
        ) == FLASH_ERROR_ALIGNMENT
    );
    assert(
        flash_erase(
            &flash,
            1U,
            16U
        ) == FLASH_ERROR_ALIGNMENT
    );
    assert(
        flash_erase(
            &flash,
            0U,
            0U
        ) == FLASH_ERROR_INVALID_ARGUMENT
    );

    const flash_byte_t programmed_data[4]   = {0xF0U, 0xFFU, 0xFFU, 0xFFU};
    assert(
        flash_program(
            &flash,
            0U,
            programmed_data,
            sizeof(programmed_data)
        ) == FLASH_NO_ERROR
    );

    const flash_byte_t attempted_restore[4] = {0xFFU, 0xFFU, 0xFFU, 0xFFU};
    assert(
        flash_program(
            &flash,
            0U,
            attempted_restore,
            sizeof(attempted_restore)
        ) == FLASH_ERROR_REQUIRES_ERASE
    );

    (void) printf("[success] %s()\n", __func__);
}

void test_queue_failed_ack_during_erase(void) {
    const registry_t first  = test_registry_create(1U, REGISTRY_NO_ERROR, 1U);
    const registry_t second = test_registry_create(2U, REGISTRY_NO_ERROR, 2U);
    registry_t actual_record;

    assert(test_initialize(&test_flash_config));
    assert(
        queue_push(
            &test.queue, &first
        ) == QUEUE_NO_ERROR
    );
    assert(
        queue_push(
            &test.queue, &second
        ) == QUEUE_NO_ERROR
    );

    assert(
        flash_inject_failure(
            &test.flash, FLASH_FAILURE_ERASE, 1U
        ) == FLASH_NO_ERROR
    );
    assert(
        queue_ack(
            &test.queue
        ) == QUEUE_ERROR_FLASH_OPERATION
    );

    assert(
        queue_peek(
            &test.queue, &actual_record
        ) == QUEUE_NO_ERROR
    );
    assert(test_registry_equal(&actual_record, &first));

    const registry_t expected_records[2] = {first, second};
    assert(test_queue_equals(expected_records, 2U));

    assert(test_reboot());
    assert(
        queue_peek(
            &test.queue, &actual_record
        ) == QUEUE_NO_ERROR
    );
    assert(test_registry_equal(&actual_record, &first));
    assert(
        queue_ack(
            &test.queue
        ) == QUEUE_NO_ERROR
    );
    assert(
        queue_peek(
            &test.queue, &actual_record
        ) == QUEUE_NO_ERROR
    );
    assert(test_registry_equal(&actual_record, &second));
    assert(
        queue_ack(
            &test.queue
        ) == QUEUE_NO_ERROR
    );
    assert(
        queue_peek(
            &test.queue, &actual_record
        ) == QUEUE_ERROR_EMPTY
    );

    (void) printf("[success] %s()\n", __func__);
}

void test_queue_failed_ack_during_program(void) {
    const registry_t first  = test_registry_create(1U, REGISTRY_NO_ERROR, 1U);
    const registry_t second = test_registry_create(2U, REGISTRY_NO_ERROR, 2U);
    registry_t actual_record;

    assert(test_initialize(&test_flash_config));
    assert(
        queue_push(
            &test.queue, &first
        ) == QUEUE_NO_ERROR
    );
    assert(
        queue_push(
            &test.queue, &second
        ) == QUEUE_NO_ERROR
    );

    assert(
        flash_inject_failure(
            &test.flash, FLASH_FAILURE_PROGRAM, 2U
        ) == FLASH_NO_ERROR
    );
    assert(
        queue_ack(
            &test.queue
        ) == QUEUE_ERROR_FLASH_OPERATION
    );

    assert(
        queue_peek(
            &test.queue, &actual_record
        ) == QUEUE_NO_ERROR
    );
    assert(test_registry_equal(&actual_record, &first));

    const registry_t expected_records[2] = {first, second};
    assert(test_queue_equals(expected_records, 2U));

    assert(test_reboot());
    assert(
        queue_peek(
            &test.queue, &actual_record
        ) == QUEUE_NO_ERROR
    );
    assert(test_registry_equal(&actual_record, &first));
    assert(
        queue_ack(
            &test.queue
        ) == QUEUE_NO_ERROR
    );
    assert(
        queue_peek(
            &test.queue, &actual_record
        ) == QUEUE_NO_ERROR
    );
    assert(test_registry_equal(&actual_record, &second));
    assert(
        queue_ack(
            &test.queue
        ) == QUEUE_NO_ERROR
    );
    assert(
        queue_peek(
            &test.queue, &actual_record
        ) == QUEUE_ERROR_EMPTY
    );

    (void) printf("[success] %s()\n", __func__);
}

void test_queue_failed_push_during_erase(void) {
    const registry_t first  = test_registry_create(1U, REGISTRY_NO_ERROR, 1U);
    const registry_t second = test_registry_create(2U, REGISTRY_NO_ERROR, 2U);
    const registry_t third  = test_registry_create(3U, REGISTRY_NO_ERROR, 3U);
    registry_t actual_record;

    assert(test_initialize(&test_flash_config));
    assert(
        queue_push(
            &test.queue, &first
        )  == QUEUE_NO_ERROR
    );
    assert(
        queue_push(
            &test.queue, &second
        ) == QUEUE_NO_ERROR
    );

    /* The inactive slot contains an older committed snapshot. */
    assert(
        flash_inject_failure(
            &test.flash, FLASH_FAILURE_ERASE, 1U
        ) == FLASH_NO_ERROR
    );
    assert(
        queue_push(
            &test.queue, &third
        ) == QUEUE_ERROR_FLASH_OPERATION
    );

    assert(
        queue_peek(
            &test.queue, &actual_record
        ) == QUEUE_NO_ERROR
    );
    assert(test_registry_equal(&actual_record, &first));

    const registry_t expected_records[2] = {first, second};
    assert(test_queue_equals(expected_records, 2U));

    assert(test_reboot());
    assert(
        queue_peek(
            &test.queue, &actual_record
        ) == QUEUE_NO_ERROR
    );
    assert(test_registry_equal(&actual_record, &first));
    assert(
        queue_ack(
            &test.queue
        ) == QUEUE_NO_ERROR
    );
    assert(
        queue_peek(
            &test.queue, &actual_record
        ) == QUEUE_NO_ERROR
    );
    assert(test_registry_equal(&actual_record, &second));
    assert(
        queue_ack(
            &test.queue
        ) == QUEUE_NO_ERROR
    );
    assert(
        queue_peek(
            &test.queue, &actual_record
        ) == QUEUE_ERROR_EMPTY
    );

    (void) printf("[success] %s()\n", __func__);
}

void test_queue_failed_push_during_program(void) {
    const registry_t first  = test_registry_create(1U, REGISTRY_NO_ERROR, 1U);
    const registry_t second = test_registry_create(2U, REGISTRY_NO_ERROR, 2U);
    const registry_t third  = test_registry_create(3U, REGISTRY_NO_ERROR, 3U);
    registry_t actual_record;

    assert(test_initialize(&test_flash_config));
    assert(
        queue_push(
            &test.queue, &first
        )  == QUEUE_NO_ERROR
    );
    assert(
        queue_push(
            &test.queue, &second
        ) == QUEUE_NO_ERROR
    );

    assert(
        flash_inject_failure(
            &test.flash, FLASH_FAILURE_PROGRAM, 3U
        ) == FLASH_NO_ERROR
    );
    assert(
        queue_push(
            &test.queue, &third
        ) == QUEUE_ERROR_FLASH_OPERATION
    );

    assert(
        queue_peek(
            &test.queue, &actual_record
        ) == QUEUE_NO_ERROR
    );
    assert(test_registry_equal(&actual_record, &first));

    const registry_t expected_records[2] = {first, second};
    assert(test_queue_equals(expected_records, 2U));

    assert(test_reboot());
    assert(
        queue_peek(
            &test.queue, &actual_record
        ) == QUEUE_NO_ERROR
    );
    assert(test_registry_equal(&actual_record, &first));
    assert(
        queue_ack(
            &test.queue
        ) == QUEUE_NO_ERROR
    );
    assert(
        queue_peek(
            &test.queue, &actual_record
        ) == QUEUE_NO_ERROR
    );
    assert(test_registry_equal(&actual_record, &second));
    assert(
        queue_ack(
            &test.queue
        ) == QUEUE_NO_ERROR
    );
    assert(
        queue_peek(
            &test.queue, &actual_record
        ) == QUEUE_ERROR_EMPTY
    );

    (void) printf("[success] %s()\n", __func__);
}

void test_queue_failed_push_before_commit(void) {
    const registry_t first  = test_registry_create(1U, REGISTRY_NO_ERROR, 1U);
    const registry_t second = test_registry_create(2U, REGISTRY_NO_ERROR, 2U);
    const registry_t third  = test_registry_create(3U, REGISTRY_NO_ERROR, 3U);
    registry_t actual_record;

    const flash_units_before_failure_t snapshot_program_units = (
        (flash_units_before_failure_t) (
            (
                QUEUE_SNAPSHOT_DATA_SIZE_BYTES +
                TEST_FLASH_PROGRAM_UNIT_SIZE_BYTES - 1U
            ) / TEST_FLASH_PROGRAM_UNIT_SIZE_BYTES
        )
    );

    assert(test_initialize(&test_flash_config));
    assert(
        queue_push(
            &test.queue, &first
        ) == QUEUE_NO_ERROR
    );
    assert(
        queue_push(
            &test.queue, &second
        ) == QUEUE_NO_ERROR
    );

    /* Allow the complete snapshot body, then fail before the commit marker. */
    assert(
        flash_inject_failure(
            &test.flash,
            FLASH_FAILURE_PROGRAM,
            snapshot_program_units
        ) == FLASH_NO_ERROR
    );
    assert(
        queue_push(
            &test.queue, &third
        ) == QUEUE_ERROR_FLASH_OPERATION
    );

    const registry_t expected_records[2] = {first, second};
    assert(test_queue_equals(expected_records, 2U));

    assert(test_reboot());
    assert(
        queue_peek(
            &test.queue, &actual_record
        ) == QUEUE_NO_ERROR
    );
    assert(test_registry_equal(&actual_record, &first));
    assert(
        queue_ack(
            &test.queue
        ) == QUEUE_NO_ERROR
    );
    assert(
        queue_peek(
            &test.queue, &actual_record
        ) == QUEUE_NO_ERROR
    );
    assert(test_registry_equal(&actual_record, &second));
    assert(
        queue_ack(
            &test.queue
        ) == QUEUE_NO_ERROR
    );
    assert(
        queue_peek(
            &test.queue, &actual_record
        ) == QUEUE_ERROR_EMPTY
    );

    (void) printf("[success] %s()\n", __func__);
}

void test_queue_failed_push_during_commit(void) {
    const flash_config_t byte_program_config = {
        .capacity_bytes     = TEST_FLASH_CAPACITY_BYTES,
        .erase_size_bytes   = TEST_FLASH_ERASE_BLOCK_SIZE_BYTES,
        .program_size_bytes = 1U
    };

    const registry_t first  = test_registry_create(1U, REGISTRY_NO_ERROR, 1U);
    const registry_t second = test_registry_create(2U, REGISTRY_NO_ERROR, 2U);
    const registry_t third  = test_registry_create(3U, REGISTRY_NO_ERROR, 3U);
    registry_t actual_record;

    const flash_units_before_failure_t units_before_marker_interruption = (
        (flash_units_before_failure_t) (QUEUE_SNAPSHOT_DATA_SIZE_BYTES + 1U)
    );

    assert(test_initialize(&byte_program_config));
    assert(
        queue_push(
            &test.queue, &first
        ) == QUEUE_NO_ERROR
    );
    assert(
        queue_push(
            &test.queue, &second
        ) == QUEUE_NO_ERROR
    );

    /* Program one commit-marker byte, then interrupt the remaining marker. */
    assert(
        flash_inject_failure(
            &test.flash,
            FLASH_FAILURE_PROGRAM,
            units_before_marker_interruption
        ) == FLASH_NO_ERROR
    );
    assert(
        queue_push(
            &test.queue, &third
        ) == QUEUE_ERROR_FLASH_OPERATION
    );

    const registry_t expected_records[2] = {first, second};
    assert(test_queue_equals(expected_records, 2U));

    assert(test_reboot());
    assert(
        queue_peek(
            &test.queue, &actual_record
        ) == QUEUE_NO_ERROR
    );
    assert(test_registry_equal(&actual_record, &first));
    assert(
        queue_ack(
            &test.queue
        ) == QUEUE_NO_ERROR
    );
    assert(
        queue_peek(
            &test.queue, &actual_record
        ) == QUEUE_NO_ERROR
    );
    assert(test_registry_equal(&actual_record, &second));
    assert(
        queue_ack(
            &test.queue
        ) == QUEUE_NO_ERROR
    );
    assert(
        queue_peek(
            &test.queue, &actual_record
        ) == QUEUE_ERROR_EMPTY
    );

    (void) printf("[success] %s()\n", __func__);
}

void test_queue_flash_insufficient(void) {
    flash_byte_t storage[1024];
    flash_t flash;
    queue_t queue;
    const flash_config_t config = {
        .capacity_bytes     = sizeof(storage),
        .erase_size_bytes   = 256U,
        .program_size_bytes = 4U
    };

    memset(storage, 0xFF, sizeof(storage));
    assert(
        flash_init(
            &flash, &config, storage, sizeof(storage)
        ) == FLASH_NO_ERROR
    );
    assert(
        queue_init(
            &queue, &flash
        ) == QUEUE_ERROR_FLASH_CONFIGURATION
    );

    (void) printf("[success] %s()\n", __func__);
}

void test_queue_full_error(void) {
    assert(test_initialize(&test_flash_config));

    registry_t expected_records[QUEUE_CAPACITY];
    for (uint32_t i = 0U; i < QUEUE_CAPACITY; i++) {
        expected_records[i] = test_registry_create(
            i, REGISTRY_NO_ERROR, 2000U + i
        );

        assert(
            queue_push(
                &test.queue, &expected_records[i]
            ) == QUEUE_NO_ERROR
        );
    }

    const registry_t rejected_registry = test_registry_create(
        99U, (registry_error_code_t ) 7, 9999U
    );
    assert(
        queue_push(
            &test.queue, &rejected_registry
        ) == QUEUE_ERROR_FULL
    );

    /* A successful ack frees one slot; the new record must join the tail. */
    assert(
        queue_ack(&test.queue) == QUEUE_NO_ERROR
    );
    assert(
        queue_push(
            &test.queue, &rejected_registry
        ) == QUEUE_NO_ERROR
    );

    registry_t expected_release[QUEUE_CAPACITY];
    for (uint32_t i = 0U; i < (QUEUE_CAPACITY - 1U); i++) {
        expected_release[i] = expected_records[i + 1U];
    }
    expected_release[QUEUE_CAPACITY - 1U] = rejected_registry;

    assert(test_reboot());
    assert(test_queue_contains_sequence(expected_release, QUEUE_CAPACITY));

    (void) printf("[success] %s()\n", __func__);
}

void test_queue_initialization_empty(void) {
    assert(test_initialize(&test_flash_config));

    registry_t registry;
    assert(
        queue_peek(
            &test.queue, &registry
        ) == QUEUE_ERROR_EMPTY
    );
    assert(
        queue_ack(
            &test.queue
        ) == QUEUE_ERROR_EMPTY
    );

    (void) printf("[success] %s()\n", __func__);
}

void test_queue_initialization_errors(void) {
    queue_t uninitialized_queue = {0};

    registry_t registry = test_registry_create(1U, REGISTRY_NO_ERROR, 1U);
    assert(
        queue_ack(
            &uninitialized_queue
        )  == QUEUE_ERROR_NOT_INITIALIZED
    );
    assert(
        queue_ack(
            NULL
        ) == QUEUE_ERROR_INVALID_ARGUMENT
    );
    assert(
        queue_init(
            NULL, NULL
        ) == QUEUE_ERROR_INVALID_ARGUMENT
    );
    assert(
        queue_peek(
            &uninitialized_queue, &registry
        ) == QUEUE_ERROR_NOT_INITIALIZED
    );
    assert(
        queue_peek(
            NULL, &registry
        ) == QUEUE_ERROR_INVALID_ARGUMENT);
    assert(
        queue_peek(
            &uninitialized_queue, NULL
        ) == QUEUE_ERROR_INVALID_ARGUMENT
    );
    assert(
        queue_push(
            NULL, &registry
        ) == QUEUE_ERROR_INVALID_ARGUMENT);
    assert(
        queue_push(
            &uninitialized_queue, NULL
        ) == QUEUE_ERROR_INVALID_ARGUMENT
    );
    assert(
        queue_push(
            &uninitialized_queue, &registry
        ) == QUEUE_ERROR_NOT_INITIALIZED
    );

    (void) printf("[success] %s()\n", __func__);
}

void test_queue_persistence(void) {
    assert(test_initialize(&test_flash_config));

    const registry_t expected_records[3] = {
        test_registry_create(1U, REGISTRY_NO_ERROR,         1001U),
        test_registry_create(2U, REGISTRY_MAX_ERROR_CODE,   1002U),
        test_registry_create(3U, REGISTRY_NO_ERROR,         1003U)
    };
    for (uint32_t i = 0U; i < 3U; i++) {
        assert(
            queue_push(
                &test.queue, &expected_records[i]
            ) ==QUEUE_NO_ERROR
        );
    }

    registry_t actual_record;
    assert(
        queue_peek(
            &test.queue, &actual_record
        ) == QUEUE_NO_ERROR
    );
    assert(test_registry_equal(&actual_record, &expected_records[0]));

    assert(test_reboot());
    assert(
        queue_peek(
            &test.queue, &actual_record
        ) == QUEUE_NO_ERROR
    );
    assert(test_registry_equal(&actual_record, &expected_records[0]));
    assert(
        queue_ack(
            &test.queue
        ) == QUEUE_NO_ERROR
    );
    assert(
        queue_peek(
            &test.queue, &actual_record
        ) == QUEUE_NO_ERROR
    );
    assert(test_registry_equal(&actual_record, &expected_records[1]));

    assert(test_reboot());
    assert(test_queue_contains_sequence(&expected_records[1], 2U));

    assert(test_reboot());
    assert(
        queue_peek(
            &test.queue, &actual_record
        ) == QUEUE_ERROR_EMPTY
    );

    (void) printf("[success] %s()\n", __func__);
}

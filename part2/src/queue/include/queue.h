/**
 * @file queue.h
 * @brief Persistent FIFO of registries.
 *
 * @author Guilherme Nunes Trofino
 * @date 2026-11-22
 */

#pragma once

#include "flash.h"
#include "queue_constants.h"
#include "registry.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Queue functions possible errors.
 */
typedef enum queue_error {
    /**
     * @brief The requested queue operation completed successfully.
     */
    QUEUE_NO_ERROR = 0,

    /**
     * @brief Argument  pointer is null or is otherwise invalid.
     */
    QUEUE_ERROR_INVALID_ARGUMENT,

    /**
     * @brief The queue has not been successfully initialized.
     */
    QUEUE_ERROR_NOT_INITIALIZED,

    /**
     * @brief The supplied registry contains an invalid serial number.
     */
    QUEUE_ERROR_INVALID_REGISTRY,

    /**
     * @brief The queue is full and the producer should apply backpressure.
     */
    QUEUE_ERROR_FULL,

    /**
     * @brief The queue does not contain a record to inspect or acknowledge.
     */
    QUEUE_ERROR_EMPTY,

    /**
     * @brief The flash handle or its configuration geometry is unsuitable.
     */
    QUEUE_ERROR_FLASH_CONFIGURATION,

    /**
     * @brief A flash erase, program or read-back verification failed.
     */
    QUEUE_ERROR_FLASH_OPERATION,

    /**
     * @brief A committed snapshot has invalid metadata or an invalid CRC.
     */
    QUEUE_ERROR_STORAGE_CORRUPT,

    /**
     * @brief The snapshot generation counter cannot be incremented safely.
     */
    QUEUE_ERROR_GENERATION_EXHAUSTED
} queue_error_t;

/**
 * @brief Identifier of the snapshot region currently selected by the queue.
 */
typedef enum queue_snapshot_slot {
    /**
     * @brief No snapshot has been committed yet.
     */
    QUEUE_SNAPSHOT_SLOT_NONE = 0,

    /**
     * @brief First half of the configured flash region.
     */
    QUEUE_SNAPSHOT_SLOT_A,

    /**
     * @brief Second half of the configured flash region.
     */
    QUEUE_SNAPSHOT_SLOT_B
} queue_snapshot_slot_t;

/**
 * @brief Generation number assigned to each committed snapshot.
 */
typedef uint32_t queue_generation_t;

/**
 * @brief Number of records currently held by a queue.
 */
typedef uint8_t queue_record_count_t;

/**
 * @brief Runtime state of the persistent FIFO.
 *
 * The fields are exposed so that callers can allocate this object without
 * dynamic memory. Applications should treat the fields as private and use the
 * queue functions to access or modify the state.
 */
typedef struct queue {
    /**
     * @brief FIFO records in oldest-to-newest order.
     */
    registry_t records[QUEUE_CAPACITY];

    /**
     * @brief Number of valid entries in records.
     */
    queue_record_count_t record_count;

    /**
     * @brief Flash region containing the newest committed snapshot.
     */
    queue_snapshot_slot_t active_slot;

    /**
     * @brief Generation number of the newest committed snapshot.
     */
    queue_generation_t generation;

    /**
     * @brief Flash device used for persistent storage.
     */
    flash_t *flash;

    /**
     * @brief Flash geometry captured during initialization.
     */
    flash_config_t flash_config;

    /**
     * @brief Indicates whether initialization completed successfully.
     */
    bool initialized;
} queue_t;

/**
 * @brief Persist removal of the oldest registry.
 *
 * The function writes and commits a snapshot without the oldest registry
 * before changing the RAM state. An empty snapshot is committed when the last
 * record is acknowledged, preventing that record from reappearing after reboot.
 *
 * @param queue Initialized queue.
 *
 * @return `QUEUE_NO_ERROR` on success; otherwise, an error status.
 */
queue_error_t queue_ack(queue_t *queue);

/**
 * @brief Return a human-readable name for a queue error value.
 *
 * @param status Queue error to describe.
 *
 * @return Static error name. Unknown values return `QUEUE_ERROR_UNKNOWN`.
 */
const char *queue_error_string(queue_error_t error);

/**
 * @brief Reconstruct the FIFO state from the configured flash region.
 *
 * The flash region is divided into two equal snapshot slots. Initialization
 * selects the newest valid committed snapshot. If neither slot contains a
 * committed snapshot, the queue is treated as empty. A snapshot with a valid
 * commit marker but invalid metadata or CRC is reported as corrupt.
 *
 * Calling this function again with a newly created queue object and the same
 * backing flash bytes simulates a reboot.
 *
 * @param queue Queue object to initialize.
 * @param flash Initialized fake flash handle.
 *
 * @return `QUEUE_NO_ERROR` on success; otherwise, an error status.
 */
queue_error_t queue_init(queue_t *queue, flash_t *flash);

/**
 * @brief Copy the oldest registry without removing it from the FIFO.
 *
 * @param queue Initialized queue.
 * @param registry Destination receiving the oldest registry.
 *
 * @return `QUEUE_NO_ERROR` on success; otherwise, an error status.
 */
queue_error_t queue_peek(const queue_t *queue, registry_t *registry);

/**
 * @brief Persist and append a new registry to the FIFO.
 *
 * The function refuses the new registry when the queue is full. It never
 * overwrites an existing record. The RAM state is updated only after the new
 * snapshot has been written, verified and committed in flash.
 *
 * @param queue Initialized queue.
 * @param registry Registry to append.
 *
 * @return `QUEUE_NO_ERROR` when the registry was committed; otherwise, an
 *         error status such as `QUEUE_ERROR_FULL` or
 *         `QUEUE_ERROR_FLASH_OPERATION`.
 */
queue_error_t queue_push(queue_t *queue, const registry_t *registry);

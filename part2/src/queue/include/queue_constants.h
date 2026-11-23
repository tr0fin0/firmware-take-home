/**
 * @file queue.h
 * @brief Persistent FIFO constants.
 *
 * @author Guilherme Nunes Trofino
 * @date 2026-11-23
 */

#pragma once

/**
 * @brief Maximum number of registries stored by the queue.
 */
#define QUEUE_CAPACITY 32U

/**
 * @brief Number of bytes used to serialize one registry to flash.
 *
 * The serialized representation is 12 serial-number bytes, one failure-code
 * byte and four timestamp bytes.
 */
#define QUEUE_SERIALIZED_REGISTRY_SIZE_BYTES 17U

/**
 * @brief Number of bytes in the queue's serialized snapshot header.
 */
#define QUEUE_SNAPSHOT_HEADER_SIZE_BYTES 11U

/**
 * @brief Number of bytes reserved for the snapshot CRC-32 value.
 */
#define QUEUE_SNAPSHOT_CRC_SIZE_BYTES 4U

/**
 * @brief Number of bytes in the fixed snapshot data before program alignment.
 */
#define QUEUE_SNAPSHOT_DATA_SIZE_BYTES                                      \
    (                                                                       \
        (QUEUE_SNAPSHOT_HEADER_SIZE_BYTES) +                                \
        (QUEUE_CAPACITY * QUEUE_SERIALIZED_REGISTRY_SIZE_BYTES) +           \
        (QUEUE_SNAPSHOT_CRC_SIZE_BYTES)                                     \
    )

/**
 * @brief Maximum snapshot data buffer size, including program-unit padding.
 */
#define QUEUE_SNAPSHOT_MAX_DATA_BUFFER_SIZE_BYTES                           \
    (QUEUE_SNAPSHOT_DATA_SIZE_BYTES + FLASH_MAX_PROGRAM_UNIT_SIZE_BYTES - 1U)

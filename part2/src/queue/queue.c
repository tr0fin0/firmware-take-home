#include "queue.h"
#include "queue_constants.h"

#include <string.h>

#define QUEUE_SNAPSHOT_CRC_OFFSET                               \
    (                                                           \
        (QUEUE_SNAPSHOT_HEADER_SIZE_BYTES) +                    \
        (QUEUE_CAPACITY * QUEUE_SERIALIZED_REGISTRY_SIZE_BYTES) \
    )

#define QUEUE_SNAPSHOT_COUNT_OFFSET         10U

#define QUEUE_SNAPSHOT_COMMIT_MARKER        0xC04D17EDUL

#define QUEUE_SNAPSHOT_MAGIC                0x51554555UL

#define QUEUE_SNAPSHOT_GENERATION_OFFSET    6U

#define QUEUE_SNAPSHOT_MAGIC_OFFSET         0U

#define QUEUE_SNAPSHOT_RECORDS_OFFSET       QUEUE_SNAPSHOT_HEADER_SIZE_BYTES

#define QUEUE_SNAPSHOT_VERSION              1U

#define QUEUE_SNAPSHOT_VERSION_OFFSET       4U

/**
 * @brief Result of inspecting one snapshot slot.
 */
typedef enum queue_snapshot_read_status {
    QUEUE_SNAPSHOT_READ_UNCOMMITTED = 0,
    QUEUE_SNAPSHOT_READ_VALID,
    QUEUE_SNAPSHOT_READ_CORRUPT
} queue_snapshot_read_status_t;

/**
 * @brief Decoded contents of one valid snapshot.
 */
typedef struct queue_snapshot {
    registry_t records[QUEUE_CAPACITY];
    queue_record_count_t record_count;
    queue_generation_t generation;
    queue_snapshot_slot_t slot;
} queue_snapshot_t;

static flash_size_t queue_align_up(
    flash_size_t value, flash_program_size_t alignment
) {
    const flash_size_t remainder = value % alignment;

    if (remainder == 0U) {
        return value;
    }

    return (value + (alignment - remainder));
}

static uint32_t queue_crc32(const flash_byte_t *data, flash_size_t size_bytes) {
    uint32_t crc = 0xFFFFFFFFUL;

    for (flash_size_t index = 0U; index < size_bytes; index++) {
        crc ^= data[index];

        for (uint8_t bit = 0U; bit < 8U; bit++) {
            if ((crc & 1UL) != 0UL) {
                crc = (crc >> 1U) ^ 0xEDB88320UL;
            } else {
                crc >>= 1U;
            }
        }
    }

    return (crc ^ 0xFFFFFFFFUL);
}

static bool queue_flash_data_valid(
    const queue_t *queue,
    flash_address_t address,
    const flash_byte_t *expected,
    flash_size_t size_bytes
) {
    flash_byte_t readback[FLASH_MAX_PROGRAM_UNIT_SIZE_BYTES];
    flash_size_t offset = 0U;

    while (offset < size_bytes) {
        const flash_size_t remaining = size_bytes - offset;
        const flash_size_t chunk_size = (
            (remaining < sizeof(readback)) ? remaining : sizeof(readback)
        );

        if (
            flash_read(
                queue->flash,
                address + offset,
                readback,
                chunk_size
            ) != FLASH_NO_ERROR
        ) {
            return false;
        }

        if (memcmp(&expected[offset], readback, chunk_size) != 0) {
            return false;
        }

        offset += chunk_size;
    }

    return true;
}

static bool queue_flash_geometry_valid(const flash_config_t *config) {
    if (
        (config == NULL) ||
        (config->capacity_bytes == 0U) ||
        (config->erase_size_bytes == 0U) ||
        (config->program_size_bytes == 0U)
    ) {
        return false;
    }

    const flash_size_t erase_block_count = (
        config->capacity_bytes / config->erase_size_bytes
    );

    if ((config->capacity_bytes % config->erase_size_bytes) != 0U) {
        return false;
    }

    if ((erase_block_count < 2U) || ((erase_block_count % 2U) != 0U)) {
        return false;
    }

    if ((config->erase_size_bytes % config->program_size_bytes) != 0U) {
        return false;
    }

    const flash_size_t slot_size_bytes = config->capacity_bytes / 2U;

    const flash_size_t snapshot_data_size_bytes = queue_align_up(
        QUEUE_SNAPSHOT_DATA_SIZE_BYTES,
        config->program_size_bytes
    );

    const flash_size_t required_slot_size = (
        config->erase_size_bytes + snapshot_data_size_bytes
    );

    return slot_size_bytes >= required_slot_size;
}

static flash_address_t queue_get_slot_address(
    const queue_t *queue, queue_snapshot_slot_t slot
) {
    const flash_size_t slot_size = queue->flash_config.capacity_bytes / 2U;

    if (slot == QUEUE_SNAPSHOT_SLOT_A) {
        return 0U;
    }

    return slot_size;
}

static queue_snapshot_slot_t queue_get_slot_inactive(const queue_t *queue) {
    if (queue->active_slot == QUEUE_SNAPSHOT_SLOT_A) {
        return QUEUE_SNAPSHOT_SLOT_B;
    }

    return QUEUE_SNAPSHOT_SLOT_A;
}

static uint16_t queue_read_u16_le(const flash_byte_t *source) {
    return (
        (uint16_t) (((uint16_t) source[0]) | (((uint16_t) source[1]) << 8U))
    );
}

static uint32_t queue_read_u32_le(const flash_byte_t *source) {
    return (
        ((uint32_t) source[0]) |
        ((uint32_t) source[1] <<  8U) |
        ((uint32_t) source[2] << 16U) |
        ((uint32_t) source[3] << 24U)
    );
}

static void queue_write_u16_le(flash_byte_t *destination, uint16_t value) {
    destination[0] = (flash_byte_t) ( value        & 0xFFU);
    destination[1] = (flash_byte_t) ((value >> 8U) & 0xFFU);
}

static void queue_write_u32_le(flash_byte_t *destination, uint32_t value) {
    destination[0] = (flash_byte_t) ( value         & 0xFFUL);
    destination[1] = (flash_byte_t) ((value >>  8U) & 0xFFUL);
    destination[2] = (flash_byte_t) ((value >> 16U) & 0xFFUL);
    destination[3] = (flash_byte_t) ((value >> 24U) & 0xFFUL);
}

static bool queue_registry_deserialize(
    const flash_byte_t *source, registry_t *registry
) {
    memcpy(
        registry->serial_number,
        source,
        REGISTRY_SERIAL_NUMBER_SIZE
    );

    registry->error_code = source[REGISTRY_SERIAL_NUMBER_SIZE];

    registry->timestamp = queue_read_u32_le(
        &source[REGISTRY_SERIAL_NUMBER_SIZE + 1U]
    );

    return true;
}

static void queue_registry_serialize(
    flash_byte_t *destination, const registry_t *registry
) {
    memcpy(
        destination,
        registry->serial_number,
        REGISTRY_SERIAL_NUMBER_SIZE
    );

    destination[REGISTRY_SERIAL_NUMBER_SIZE] = registry->error_code;

    queue_write_u32_le(
        &destination[REGISTRY_SERIAL_NUMBER_SIZE + 1U],
        registry->timestamp
    );
}

static queue_snapshot_read_status_t queue_snapshot_read(
    const queue_t *queue,
    queue_snapshot_slot_t slot,
    queue_snapshot_t *snapshot
) {
    flash_byte_t data_buffer[QUEUE_SNAPSHOT_MAX_DATA_BUFFER_SIZE_BYTES];
    flash_byte_t commit_buffer[FLASH_MAX_PROGRAM_UNIT_SIZE_BYTES];

    const flash_config_t *config = &queue->flash_config;

    const flash_address_t slot_address = queue_get_slot_address(queue, slot);

    const flash_size_t data_size = queue_align_up(
        QUEUE_SNAPSHOT_DATA_SIZE_BYTES,
        config->program_size_bytes
    );

    const flash_size_t commit_size = queue_align_up(
        (flash_size_t) sizeof(uint32_t),
        config->program_size_bytes
    );

    const flash_address_t data_address = (
        slot_address + config->erase_size_bytes
    );

    if (
        flash_read(
            queue->flash,
            slot_address,
            commit_buffer,
            commit_size
        ) != FLASH_NO_ERROR
    ) {
        return QUEUE_SNAPSHOT_READ_CORRUPT;
    }

    if (queue_read_u32_le(commit_buffer) != QUEUE_SNAPSHOT_COMMIT_MARKER) {
        return QUEUE_SNAPSHOT_READ_UNCOMMITTED;
    }

    if (
        flash_read(
            queue->flash,
            data_address,
            data_buffer,
            data_size
        ) != FLASH_NO_ERROR
    ) {
        return QUEUE_SNAPSHOT_READ_CORRUPT;
    }

    if (
        queue_read_u32_le(
            &data_buffer[QUEUE_SNAPSHOT_MAGIC_OFFSET]
        ) != QUEUE_SNAPSHOT_MAGIC
    ) {
        return QUEUE_SNAPSHOT_READ_CORRUPT;
    }

    if (
        queue_read_u16_le(
            &data_buffer[QUEUE_SNAPSHOT_VERSION_OFFSET]
        ) != QUEUE_SNAPSHOT_VERSION
    ) {
        return QUEUE_SNAPSHOT_READ_CORRUPT;
    }

    const queue_generation_t generation = queue_read_u32_le(
        &data_buffer[QUEUE_SNAPSHOT_GENERATION_OFFSET]
    );

    const queue_record_count_t record_count = (
        data_buffer[QUEUE_SNAPSHOT_COUNT_OFFSET]
    );

    if ((generation == 0U) || (record_count > QUEUE_CAPACITY)) {
        return QUEUE_SNAPSHOT_READ_CORRUPT;
    }

    const uint32_t expected_crc = queue_read_u32_le(
        &data_buffer[QUEUE_SNAPSHOT_CRC_OFFSET]
    );
    const uint32_t calculated_crc = queue_crc32(
        data_buffer, QUEUE_SNAPSHOT_CRC_OFFSET
    );

    if (expected_crc != calculated_crc) {
        return QUEUE_SNAPSHOT_READ_CORRUPT;
    }

    for (queue_record_count_t index = 0U; index < record_count; ++index) {
        const flash_size_t record_offset = (
            QUEUE_SNAPSHOT_RECORDS_OFFSET +
            ((flash_size_t) index * QUEUE_SERIALIZED_REGISTRY_SIZE_BYTES)
        );

        if (
            !queue_registry_deserialize(
                &data_buffer[record_offset],
                &snapshot->records[index]
            )
        ) {
            return QUEUE_SNAPSHOT_READ_CORRUPT;
        }
    }

    snapshot->record_count  = record_count;
    snapshot->generation    = generation;
    snapshot->slot          = slot;

    return QUEUE_SNAPSHOT_READ_VALID;
}

static queue_error_t queue_snapshot_write(
    const queue_t *queue,
    const registry_t *records,
    queue_record_count_t record_count,
    queue_snapshot_slot_t *written_slot,
    queue_generation_t *written_generation
) {
    flash_byte_t data_buffer[QUEUE_SNAPSHOT_MAX_DATA_BUFFER_SIZE_BYTES];
    flash_byte_t commit_buffer[FLASH_MAX_PROGRAM_UNIT_SIZE_BYTES];

    const flash_config_t *config = &queue->flash_config;

    const flash_size_t slot_size = config->capacity_bytes / 2U;

    const flash_size_t data_size = queue_align_up(
        QUEUE_SNAPSHOT_DATA_SIZE_BYTES,
        config->program_size_bytes
    );

    const flash_size_t commit_size = queue_align_up(
        (flash_size_t)sizeof(uint32_t),
        config->program_size_bytes
    );

    const queue_snapshot_slot_t target_slot = queue_get_slot_inactive(queue);

    const flash_address_t slot_address = queue_get_slot_address(
        queue, target_slot
    );

    const flash_address_t data_address = (
        slot_address + config->erase_size_bytes
    );

    const flash_address_t commit_address = slot_address;

    const queue_generation_t new_generation = queue->generation + 1U;

    memset(data_buffer, 0xFF, data_size);

    queue_write_u32_le(
        &data_buffer[QUEUE_SNAPSHOT_MAGIC_OFFSET],
        QUEUE_SNAPSHOT_MAGIC
    );

    queue_write_u16_le(
        &data_buffer[QUEUE_SNAPSHOT_VERSION_OFFSET],
        QUEUE_SNAPSHOT_VERSION
    );

    queue_write_u32_le(
        &data_buffer[QUEUE_SNAPSHOT_GENERATION_OFFSET],
        new_generation
    );

    data_buffer[QUEUE_SNAPSHOT_COUNT_OFFSET] = record_count;

    for (queue_record_count_t index = 0U; index < record_count; ++index) {
        const flash_size_t record_offset = (
            QUEUE_SNAPSHOT_RECORDS_OFFSET +
            ((flash_size_t) index * QUEUE_SERIALIZED_REGISTRY_SIZE_BYTES)
        );

        queue_registry_serialize(
            &data_buffer[record_offset],
            &records[index]
        );
    }

    const uint32_t crc = queue_crc32(
        data_buffer, QUEUE_SNAPSHOT_CRC_OFFSET
    );

    queue_write_u32_le(
        &data_buffer[QUEUE_SNAPSHOT_CRC_OFFSET],
        crc
    );

    /**
     * The commit marker occupies the first erase block of each slot. Erasing
     * the slot therefore invalidates its old marker before later blocks that
     * contain snapshot data are erased. This also protects recovery if erase
     * of the marker block itself is interrupted.
     */
    if (
        flash_erase(
            queue->flash,
            slot_address,
            slot_size
        ) != FLASH_NO_ERROR
    ) {
        return QUEUE_ERROR_FLASH_OPERATION;
    }

    if (
        flash_program(
            queue->flash,
            data_address,
            data_buffer,
            data_size
        ) != FLASH_NO_ERROR
    ) {
        return QUEUE_ERROR_FLASH_OPERATION;
    }

    if (
        !queue_flash_data_valid(
            queue,
            data_address,
            data_buffer,
            data_size
        )
    ) {
        return QUEUE_ERROR_FLASH_OPERATION;
    }

    memset(commit_buffer, 0xFF, commit_size);
    queue_write_u32_le(commit_buffer, QUEUE_SNAPSHOT_COMMIT_MARKER);

    if (
        flash_program(
            queue->flash,
            commit_address,
            commit_buffer,
            commit_size
        ) != FLASH_NO_ERROR
    ) {
        return QUEUE_ERROR_FLASH_OPERATION;
    }

    if (
        !queue_flash_data_valid(
            queue,
            commit_address,
            commit_buffer,
            commit_size
        )
    ) {
        return QUEUE_ERROR_FLASH_OPERATION;
    }

    *written_slot = target_slot;
    *written_generation = new_generation;

    return QUEUE_NO_ERROR;
}

queue_error_t queue_ack(queue_t *queue) {
    if (queue == NULL) {
        return QUEUE_ERROR_INVALID_ARGUMENT;
    }

    if (!queue->initialized) {
        return QUEUE_ERROR_NOT_INITIALIZED;
    }

    if (queue->record_count == 0U) {
        return QUEUE_ERROR_EMPTY;
    }

    if (queue->generation == UINT32_MAX) {
        return QUEUE_ERROR_GENERATION_EXHAUSTED;
    }

    registry_t candidate_records[QUEUE_CAPACITY];
    memset(candidate_records, 0, sizeof(candidate_records));

    const queue_record_count_t candidate_count = (
        (queue_record_count_t) (queue->record_count - 1U)
    );

    for (queue_record_count_t index = 0U; index < candidate_count; index++) {
        candidate_records[index] = queue->records[index + 1U];
    }

    queue_snapshot_slot_t written_slot      = QUEUE_SNAPSHOT_SLOT_NONE;
    queue_generation_t written_generation   = 0U;

    const queue_error_t persist_status = queue_snapshot_write(
        queue,
        candidate_records,
        candidate_count,
        &written_slot,
        &written_generation
    );

    if (persist_status != QUEUE_NO_ERROR) {
        return persist_status;
    }

    memset(queue->records, 0, sizeof(queue->records));
    memcpy(
        queue->records,
        candidate_records,
        ((size_t)candidate_count * sizeof(registry_t))
    );
    queue->record_count = candidate_count;
    queue->active_slot  = written_slot;
    queue->generation   = written_generation;

    return QUEUE_NO_ERROR;
}

const char *queue_error_string(queue_error_t error) {
    switch (error) {
        case QUEUE_NO_ERROR:
            return "QUEUE_NO_ERROR";

        case QUEUE_ERROR_INVALID_ARGUMENT:
            return "QUEUE_ERROR_INVALID_ARGUMENT";

        case QUEUE_ERROR_NOT_INITIALIZED:
            return "QUEUE_ERROR_NOT_INITIALIZED";

        case QUEUE_ERROR_INVALID_REGISTRY:
            return "QUEUE_ERROR_INVALID_REGISTRY";

        case QUEUE_ERROR_FULL:
            return "QUEUE_ERROR_FULL";

        case QUEUE_ERROR_EMPTY:
            return "QUEUE_ERROR_EMPTY";

        case QUEUE_ERROR_FLASH_CONFIGURATION:
            return "QUEUE_ERROR_FLASH_CONFIGURATION";

        case QUEUE_ERROR_FLASH_OPERATION:
            return "QUEUE_ERROR_FLASH_OPERATION";

        case QUEUE_ERROR_STORAGE_CORRUPT:
            return "QUEUE_ERROR_STORAGE_CORRUPT";

        case QUEUE_ERROR_GENERATION_EXHAUSTED:
            return "QUEUE_ERROR_GENERATION_EXHAUSTED";

        default:
            return "QUEUE_STATUS_UNKNOWN";
    }
}

queue_error_t queue_init(queue_t *queue, flash_t *flash) {
    if ((queue == NULL) || (flash == NULL)) {
        return QUEUE_ERROR_INVALID_ARGUMENT;
    }

    memset(queue, 0, sizeof(*queue));

    flash_config_t config;
    if (flash_get_config(flash, &config) != FLASH_NO_ERROR) {
        return QUEUE_ERROR_FLASH_CONFIGURATION;
    }

    if (!queue_flash_geometry_valid(&config)) {
        return QUEUE_ERROR_FLASH_CONFIGURATION;
    }

    queue->flash = flash;
    queue->flash_config = config;

    queue_snapshot_t snapshot_a;
    queue_snapshot_t snapshot_b;

    const queue_snapshot_read_status_t status_a = queue_snapshot_read(
        queue,
        QUEUE_SNAPSHOT_SLOT_A,
        &snapshot_a
    );
    const queue_snapshot_read_status_t status_b = queue_snapshot_read(
        queue,
        QUEUE_SNAPSHOT_SLOT_B,
        &snapshot_b
    );

    if (
        (status_a == QUEUE_SNAPSHOT_READ_CORRUPT) ||
        (status_b == QUEUE_SNAPSHOT_READ_CORRUPT)
    ) {
        return QUEUE_ERROR_STORAGE_CORRUPT;
    }

    if (
        (status_a == QUEUE_SNAPSHOT_READ_UNCOMMITTED) &&
        (status_b == QUEUE_SNAPSHOT_READ_UNCOMMITTED)
    ) {
        queue->active_slot  = QUEUE_SNAPSHOT_SLOT_NONE;
        queue->generation   = 0U;
        queue->record_count = 0U;
        queue->initialized  = true;

        return QUEUE_NO_ERROR;
    }

    const queue_snapshot_t *selected_snapshot = NULL;

    if (
        (status_a == QUEUE_SNAPSHOT_READ_VALID) &&
        (status_b == QUEUE_SNAPSHOT_READ_VALID)
    ) {
        if (snapshot_a.generation == snapshot_b.generation) {
            return QUEUE_ERROR_STORAGE_CORRUPT;
        }

        selected_snapshot = (
            (snapshot_a.generation > snapshot_b.generation)
                ? &snapshot_a
                : &snapshot_b
        );
    } else if (status_a == QUEUE_SNAPSHOT_READ_VALID) {
        selected_snapshot = &snapshot_a;
    } else {
        selected_snapshot = &snapshot_b;
    }

    memcpy(
        queue->records,
        selected_snapshot->records,
        ((size_t)selected_snapshot->record_count * sizeof(registry_t))
    );
    queue->record_count = selected_snapshot->record_count;
    queue->generation   = selected_snapshot->generation;
    queue->active_slot  = selected_snapshot->slot;
    queue->initialized  = true;

    return QUEUE_NO_ERROR;
}

queue_error_t queue_peek(const queue_t *queue, registry_t *registry) {
    if ((queue == NULL) || (registry == NULL)) {
        return QUEUE_ERROR_INVALID_ARGUMENT;
    }

    if (!queue->initialized) {
        return QUEUE_ERROR_NOT_INITIALIZED;
    }

    if (queue->record_count == 0U) {
        return QUEUE_ERROR_EMPTY;
    }

    *registry = queue->records[0];
    return QUEUE_NO_ERROR;
}

queue_error_t queue_push(queue_t *queue, const registry_t *registry) {
    if ((queue == NULL) || (registry == NULL)) {
        return QUEUE_ERROR_INVALID_ARGUMENT;
    }

    if (!queue->initialized) {
        return QUEUE_ERROR_NOT_INITIALIZED;
    }

    if (queue->record_count >= QUEUE_CAPACITY) {
        return QUEUE_ERROR_FULL;
    }

    if (queue->generation == UINT32_MAX) {
        return QUEUE_ERROR_GENERATION_EXHAUSTED;
    }

    registry_t candidate_records[QUEUE_CAPACITY];
    memset(candidate_records, 0, sizeof(candidate_records));

    memcpy(
        candidate_records,
        queue->records,
        ((size_t)queue->record_count * sizeof(registry_t))
    );
    candidate_records[queue->record_count] = *registry;

    queue_snapshot_slot_t written_slot      = QUEUE_SNAPSHOT_SLOT_NONE;
    queue_generation_t written_generation   = 0U;

    const queue_error_t persist_status = queue_snapshot_write(
        queue,
        candidate_records,
        (queue_record_count_t)(queue->record_count + 1U),
        &written_slot,
        &written_generation
    );

    if (persist_status != QUEUE_NO_ERROR) {
        return persist_status;
    }

    memcpy(queue->records, candidate_records, sizeof(candidate_records));
    ++queue->record_count;
    queue->active_slot  = written_slot;
    queue->generation   = written_generation;

    return QUEUE_NO_ERROR;
}

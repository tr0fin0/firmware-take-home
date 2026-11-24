# Part 2

## Assumptions

Assumptions applicable for both parts are:

- commit messages: conventional commits

- development strategy: feature branches with squashed PRs linked to issues.

- registry serial number: contains exactly 12 ASCII alphanumeric characters: `0-9`, `A-Z`, and `a-z`, stored as a fixed-width array without a null terminator.

- registry error: `0x00` represents a successful test and `0x01 - 0xFF` are reserved for application-specific failure codes.

- registry timestamp: an opaque uint32_t.

- queue: stores up to 32 registries. When full, push rejects a new registry and returns `QUEUE_STATUS_ERROR_FULL`. The producer is responsible for applying backpressure. Existing records are never overwritten to make room.

## Build and Run

### Requirements

On Debian/Ubuntu, install the required dependencies:

```bash
sudo apt update && sudo apt install -y build-essential cmake
```

### Build

From the repository root run:

```bash
cd part2
```

Then configure and compile the project:

```bash
cmake -S . -B build && cmake --build build
```

### Run

Execute the generated binary:

```bash
./build/part2
```

### Clean

Remove generated build files:

```bash
rm -rf build
```

## Implementation

The implementation is divided into three modules:

- **registry**: defines a production test result. Each registry is serialized explicitly into 17 bytes: 12 serial-number bytes, one error-code byte and four little-endian timestamp bytes. The implementation does not write `sizeof(registry_t)` to flash.

- **flash**: provides a configurable RAM-backed fake flash. Its configuration defines capacity, erase-block size and programming-unit size. It models erased bytes (0xFF), programming from 1 to 0, erase-block operations and injectable partial program/erase failures. The backing byte array is caller-owned so it can be retained when the flash and queue handles are recreated to simulate a reboot.

- **queue**: implements a fixed-capacity FIFO of 32 registries. It exposes `queue_init()`, `queue_push()`, `queue_peek()` and `queue_ack()`, returning a `queue_error_t` value for each operation.

### Details

#### Persistence

The queue uses double-buffered snapshots. Flash is divided into two equal slots; each slot stores a complete queue snapshot. A snapshot contains a magic number, format version, generation number, record count, registry data and CRC-32. The registry fields are serialized explicitly, independently of C structure padding and enum size.

For each `queue_push()` or `queue_ack()`, the queue writes the proposed state to the inactive slot. It erases that slot, programs and verifies the snapshot data, then programs and verifies a commit marker. The active slot is left untouched during this process. The RAM queue is updated only after the new snapshot has been committed successfully.

During `queue_init()` , both slots are inspected. The newest valid committed snapshot is used to reconstruct the FIFO. If neither slot has a committed snapshot, the queue starts empty. A committed snapshot with invalid metadata or CRC is reported as storage corruption.

#### Failure handling

A failed erase, program or verification returns an error without changing the logical queue state in RAM. The previously committed snapshot remains available. The inactive slot may contain partially erased or programmed data, which is not treated as committed without a valid commit marker.

This guarantees logical queue consistency; it does not guarantee that every byte in flash remains physically unchanged after a failed operation. The fake flash injects failures between complete programming units or erase blocks, not within an individual unit.

#### Tests

The host test suite contains 18 test cases. It covers registry validation, flash configuration and operation errors, failure injection, FIFO ordering, reboot recovery, queue-full behavior, interrupted push and ack operations, commit-marker interruption, CRC validation and status-string mappings. Failure tests check both the RAM state and the state recovered after reinitialization with the same backing flash bytes.

### AI Usage

AI tools were used to discuss architectural decisions, generate and refactor C code, improve Doxygen documentation, and assist with test development.

Generated code and suggestions were reviewed and adapted. The implementation was compiled and tested using GCC.

### Future Improvements

### Tasks

- implement queue in flash
- implement `ack`
  - queue only removes entry after succesfull acknowledgement
- implement `init`
  - if reboot, nothing changes in the flash
- implement `peek`
- implement `push`
  - if fail, return error and nothing changes in the flash
- run tests

## Sources

JIG

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

The serial number is assumed to contain only letters and numbers encoded in ASCII. Therefore each character would use only 1 byte of memory.

The timestamp is assumed to follow ISO 8601 formatting.

The error code is assumed to have 255 possible error descriptions and 1 no error description.

When the persistent queue reaches its maximum capacity, new records are rejected without modifying existing records. The producer is responsible for applying backpressure and preventing additional tests from starting until storage becomes available. Successfully persisted records are never overwritten or discarded before a successful acknowledgement.

### AI Usage

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

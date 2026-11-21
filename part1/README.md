# Part 1

## Assumptions

Assumptions applicable for both parts are:

- commit messages: conventional commits
- development strategy: feature branches with squashed PRs linked to issues.

## Build and Run

### Requirements

On Debian/Ubuntu, install the required dependencies:

```bash
sudo apt update && sudo apt install -y build-essential cmake
```

### Build

From the repository root run:

```bash
cd part1
```

Then configure and compile the project:

```bash
cmake -S . -B build && cmake --build build
```

### Run

Execute the generated binary:

```bash
./build/part1
```

### Clean

Remove generated build files:

```bash
rm -rf build
```

## Implementation

## Tasks

- implement `encode`
- implement `decode`
- define error codes
- define treatment for values outside limits
- define treatment for reserved bits
- run tests

## Example

## Sources

[CRC](https://en.wikipedia.org/wiki/Cyclic_redundancy_check)

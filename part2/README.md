# Part 2

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

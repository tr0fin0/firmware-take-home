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

Three classes were implemented in C:

- `crc8`

- `payload`

- `test`

C was chosen because was the language I'm most confortable at the moment. Therefore many Objected Oriented expressions should be considered withing the capabilities of C.

The `crc8` "class" was separated from the `payload` implementation to split resposability and allow reusability of them independently specially with different correction characteristics.

The `payload_t` "class" provides an object abstraction of the different BLE fields required, the `payload_decode()` and `payload_encode()` functions provide ways to respectively decode an array of bytes and encode an array of bytes from the a `payload_t` object.

Both `payload_print()` and `payload_print_error()` functions are used to inspect `payload_t` objects and their possible erros.

As described in the `payload_types.h` file, there are 9 errors conditions and 1 error free condition. Those are used to indicate the correct behavior of the `payload_decode()` and `payload_encode()` functions. During either decoding or encoding, the functions will stop without updating their respectives outputs at the first error condition encountered to avoid providing the user with uncertain or incorrect information.

It was chosen that the user should not be able to use any information if the system is not 100\% sure of the accuracy to avoid incorrect values triggering an incorrect action.

Testing was created to validate the API and error messages. Nothing to clear here.

### AI Usage

AI helped me with the following which was accepted:

- build the CMakeLists.txt file for compilation

  - **how I checked**: running locally the configurations purposed

- write doxygen messages

  - **how I checked**: reviewing the wording

- write tests

  - **how I checked**: manually checked with paper

### Future Improvements

With more time I would like to :

- create a function that given a buffer, the starting and ending absolute bits returned the value extracted from the buffer entries

  - **why**: make code easier to change when protocol change

- verify in depth the flow encoding precision by trying different methods

  - **why**: ensure better correlation of data

### Tasks

- implement `encode`
- implement `decode`
- define error codes
- define treatment for values outside limits
- define treatment for reserved bits
- run tests


## Sources

[CRC](https://en.wikipedia.org/wiki/Cyclic_redundancy_check)

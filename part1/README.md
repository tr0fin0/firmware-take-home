# Part 1

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

### Assumptions

It was chosen that the user should not be able to use any information if the system is not 100% sure of the accuracy to avoid incorrect values triggering an incorrect action. Therefore, both the `payload_decode()` and `payload_encode()` functions would only update their respective output pointers when all the information was within the accepted limits.

### Details

The implementation is divided into two modules with a separate module for testing:

- `crc8`
- `payload`
- `test`

The `crc8` module provides an independent implementation of the CRC-8 algorithm. The `crc8_compute()` function receives a configuration structure containing the initial value, polynomial, input and output reflection settings, and final XOR value. This separation allows the CRC implementation to be reused independently of the payload.

The `payload` module provides an abstraction of the BLE advertising payload through the `payload_t` structure. Its implementation is divided into smaller functions responsible for encoding and decoding each individual field.

The `payload_decode()` and `payload_encode()` functions provide the public interface to respectively convert an array of bytes into a `payload_t` structure and generate an array of bytes from a `payload_t` object.

The different types and error conditions are defined in `payload_types.h`, while `payload_constants.h` contains the accepted minimum and maximum values for battery voltage and temperature.

The implementation performs the following validations:

- Payload version and device type.
- Battery voltage limits (1800–3600 mV).
- Temperature limits (-400–850 deci-degrees Celsius).
- Flow exponent validity.
- Reserved bits (expected value: zero).
- CRC-8 checksum integrity during decoding.

The payload functions return an error code indicating the first validation failure encountered. This allows the caller to identify why an encoding or decoding operation was unsuccessful.

The `payload_print()` and `payload_print_error()` functions provide basic ways to inspect decoded payloads and their respective error conditions.

The `test` module contains the test functions used to validate the implementation. The tests use the standard C `assert()` mechanism and cover successful encoding and decoding, invalid field values, CRC verification, and all possible combinations of the three status flags.

The reference payload provided in the exercise is also used to verify the decoding and encoding process.

#### Future Improvements

With more time I would like to:

- Improve the test coverage for flow encoding and decoding, especially around exponent transitions and rounding.

  - **Why:** Ensure the custom floating-point representation behaves correctly across its entire valid range.

- Introduce a more structured test framework.

  - **Why:** Improve test organization, reporting, and integration with automated testing.

### Tasks

- implement `encode`
- implement `decode`
- define error codes
- define treatment for values outside limits
- define treatment for reserved bits
- run tests

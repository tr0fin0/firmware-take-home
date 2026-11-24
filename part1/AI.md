# AI Usage

All generated suggestions were reviewed before being incorporated into the project.

## Accepted Contributions

### CMake configuration

- **what:** draft the `CMakeLists.txt` file and organize the compilation configuration.
- **how I checked:** ran the proposed configuration and build commands locally.
- **why:** to provide a reproducible build process using standard Linux dependencies.

### CRC calculations

- **what:** create the `crc8_compute()` function from the `crc8_config` structure.
- **how I checked:** compared the results with an online calculator.
- **why:** I had limited experience designing CRC algorithm and wanted to evaluate ensure correct implementation.

### Doxygen documentation

- **what:** draft Doxygen descriptions for public types, structures, enumerations, and functions.
- **how I checked:** reviewed the generated descriptions against the actual function behavior, types and parameters.
- **why:** to reduce documentation time and focus more on implementation and verification.

### Test development

- **what:** propose unit test cases covering normal operation, boundary conditions, and invalid inputs.
- **how I checked:** manually derived and checked the expected values against the payload specification, bit positions, and mathematical formulas.
- **why:** to improve test coverage, particularly for error conditions.

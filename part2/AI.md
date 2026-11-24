# AI Usage

All generated suggestions were reviewed before being incorporated into the project.

## Accepted Contributions

### Architectural design

- **what**: explore storage architectures including the double-buffered snapshot strategy.
- **how I checked:** reviewed the proposed module responsibilities and dependencies, then manually exercised the queue operations.
- **why:** I had limited experience designing a persistent storage abstraction and wanted to evaluate possible approaches.

### CMake configuration

- **what:** draft the `CMakeLists.txt` file and organize the compilation configuration.
- **how I checked:** ran the proposed configuration and build commands locally.
- **why:** to provide a reproducible build process using standard Linux dependencies.

### Code generation

- **what**: generate the initial implementation of the fake flash and persistent queue modules based on the agreed architecture.
- **how I checked:** compared the generated code against the agreed architecture, public API requirements and storage constraints.
- **why:** to accelerate implementation while keeping the code aligned with the previously established design decisions.

### Doxygen documentation

- **what:** draft Doxygen descriptions for public types, structures, enumerations, and functions.
- **how I checked:** Reviewed the generated descriptions against the actual function behavior, types and parameters.
- **why:** to reduce documentation time and focus more on implementation and verification.

### Test development

- **what:** propose unit test cases covering normal operation, boundary conditions, and invalid inputs.
- **how I checked:** reviewed the test scenarios and executed the suite, checking return codes and queue contents.
- **why:** to improve test coverage, particularly for persistence, FIFO ordering and recovery from interrupted flash operations.

## Rejected Contributions

The following AI suggestions were not accepted:

### Registry verification

- **why:** although registry validation is important, some suggested validation criteria were not specified by the assessment.

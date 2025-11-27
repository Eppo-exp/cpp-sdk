# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.0]

### Changed

- **BREAKING**: `ConfigurationStore::getConfiguration()` now returns `std::shared_ptr<const Configuration>` instead of `Configuration` by value
  - Returns an empty configuration when no configuration is set (graceful behavior, no null checks required)
  - Configuration is immutable (`const`) once retrieved
  - Update your code: use `->` instead of `.` to access configuration methods (no null checks needed)
- **BREAKING**: All error handling is now done through logging and returning default values
- **BREAKING**: Removed `setIsGracefulFailureMode()` method - SDK always operates in "graceful" mode
- **BREAKING**: Removed custom exception classes (`FlagConfigurationNotFoundException`, `FlagNotEnabledException`, `SubjectAllocationException`)
- **BREAKING**: `EppoClient` constructor now takes `ConfigurationStore&` by reference instead of `std::shared_ptr<ConfigurationStore>`, allowing SDK consumers to choose their own synchronization strategy
- SDK now builds with `-fno-exceptions` and does not use exceptions internally
- Configured nlohmann/json with `JSON_NOEXCEPTION` to eliminate JSON parsing exceptions
- Constructor preconditions (null checks, size validations) now use `assert()` instead of throwing exceptions
- Internal functions now return `std::optional` or error codes instead of throwing exceptions

### Added

- Full compatibility with `-fno-exceptions` projects
- Documentation on error handling and constructor preconditions

### Migration Guide

If you were using v1.0.0:

1. Remove all `try-catch` blocks around SDK method calls - they no longer throw
2. Remove calls to `setIsGracefulFailureMode()` - this method no longer exists
3. Update `EppoClient` instantiation - pass `ConfigurationStore` by reference instead of as `shared_ptr`:
   - Before: `auto client = std::make_shared<EppoClient>(configStorePtr);`
   - After: `EppoClient client(configStore);` (ensure `configStore` outlives `client`)
4. Ensure constructor preconditions are met (non-null loggers, positive cache sizes)
5. Monitor errors through the `ApplicationLogger` interface instead of catching exceptions

## [1.0.0] - 2025-11-14

Initial release of the Eppo C++ SDK.

### Added

- Feature flag evaluation: `getBoolAssignment()`, `getStringAssignment()`, `getNumericAssignment()`, `getIntegerAssignment()`, `getJSONAssignment()`, `getSerializedJSONAssignment()`
- Contextual bandits: `getBanditAction()` with ML-powered variant selection
- Evaluation details: `getBoolAssignmentDetails`, `getStringAssignmentDetails()`, `getNumericAssignmentDetails()`, `getIntegerAssignmentDetails()`, `getJSONAssignmentDetails()`, `getSerializedJSONAssignmentDetails`, `getBanditActionDetails()`
- Assignment and bandit logger interfaces with automatic deduplication
- Application logger for debug/info/warn/error messages
- Graceful failure mode with `setIsGracefulFailureMode()` for configurable error handling
- Offline configuration store for flags and bandit models
- CMake and Makefile build support
- Pre-built binaries for Linux (x86_64, aarch64), macOS (x86_64, arm64, universal), Windows (x86_64, arm64)
- Example applications for flags and bandits
- Comprehensive documentation and README

[1.0.0]: https://github.com/Eppo-exp/cpp-sdk/releases/tag/v1.0.0

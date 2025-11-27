# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- `ConfigurationStore` convenience API for setting configuration by value:
  - `ConfigurationStore(Configuration config)` - constructor accepting Configuration by value
  - `setConfiguration(Configuration config)` - setter accepting Configuration by value
- Move constructor and move assignment operator for `Configuration` class

### Changed

- **BREAKING**: `ConfigurationStore::getConfiguration()` now returns `std::shared_ptr<const Configuration>` instead of `Configuration` by value
  - Eliminates expensive configuration copies on every flag evaluation
- **BREAKING**: `Configuration` constructors now take parameters by value for better performance
- `ConfigurationStore` now uses atomic operations instead of mutex internally for better performance

### Removed

- **BREAKING**: `Configuration::precompute()` removed from public API
  - It is now called automatically in constructors, so manual invocation is no longer needed

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

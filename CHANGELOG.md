# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Changed

- **BREAKING**: `ConfigurationStore::getConfiguration()` now returns `std::shared_ptr<const Configuration>` instead of `Configuration` by value
  - Returns an empty configuration when no configuration is set (graceful behavior, no null checks required)
  - Eliminates expensive configuration copies on every flag evaluation
  - Provides thread-safe reference counting via `std::shared_ptr`
  - Configuration is immutable (`const`) once retrieved
  - Update your code: use `->` instead of `.` to access configuration methods (no null checks needed)

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

# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

**Note**: Semantic Versioning applies to source-level compatibility only. No guarantees are made regarding Application Binary Interface (ABI) stability. When upgrading versions, always recompile your application against the new SDK version. Avoid mixing object files or libraries compiled against different SDK versions.

## [Unreleased]

## [2.0.0] - 2025-12-02

### Added

- `EvaluationClient` - lightweight client for flag evaluation without logging
- `ConfigurationStore` convenience API for setting configuration by value:
  - `ConfigurationStore(Configuration config)` - constructor accepting Configuration by value
  - `setConfiguration(Configuration config)` - setter accepting Configuration by value
- Move constructor and move assignment operator for `Configuration` class
- `parseConfiguration()` convenience function for simplified configuration parsing:
  - `parseConfiguration(flagConfigJson, error)` - parse flags only with error feedback
  - `parseConfiguration(flagConfigJson, banditModelsJson, error)` - parse flags and bandits together
  - Provides simple string-based error handling for common use cases
  - Built on top of `ParseResult<T>` for structured error collection
- `parseConfigResponse()` and `parseBanditResponse()` functions returning `ParseResult<T>`:
  - Support parsing from both JSON strings and input streams
  - Collect all parsing errors instead of failing on first error
  - Enable partial success handling (return value with warnings)
- New `ParseResult<T>` template for structured error reporting during parsing
- Support for building without exceptions via `-fno-exceptions` flag
- Abort-free configuration parsing - all parsing operations can now fail gracefully

### Changed

- **BREAKING**: `ConfigurationStore::getConfiguration()` now returns `std::shared_ptr<const Configuration>` instead of `Configuration` by value
  - Eliminates expensive configuration copies on every flag evaluation
- **BREAKING**: `Configuration` constructors now take parameters by value for better performance
- **BREAKING**: SDK now uses RE2 regex library instead of `std::regex`
  - RE2 is not vulnerable to ReDoS (Regular Expression Denial of Service) attacks
  - RE2 works without exceptions, enabling exception-free builds
  - This change improves security and reliability
- `ConfigurationStore` now uses atomic operations instead of mutex internally for better performance
- **BREAKING**: Parsing functions now report errors instead of silently skipping invalid entries:
  - `parseConfigResponse()` and `parseBanditResponse()` return `ParseResult<T>` with error collection
  - Use the new `parseConfiguration()` convenience function for simplified error handling
  - Errors are aggregated and returned rather than causing silent data loss

### Removed

- **BREAKING**: `Configuration::precompute()` removed from public API
  - It is now called automatically in constructors, so manual invocation is no longer needed
- **BREAKING**: `from_json()` for `ConfigResponse` and `BanditResponse` removed
  - Replaced with `parseConfigResponse()` and `parseBanditResponse()` for better error feedback
  - Use `parseConfiguration()` convenience function for most use cases

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

[2.0.0]: https://github.com/Eppo-exp/cpp-sdk/releases/tag/v2.0.0
[1.0.0]: https://github.com/Eppo-exp/cpp-sdk/releases/tag/v1.0.0

# Building the C++ SDK

## Requirements

- C++17 compatible compiler (g++ or clang++)
- C99 compatible compiler (gcc or clang)
- Make

**No external dependencies required!** All hashing is done using vendored libraries in `third_party/`.

## Building

```bash
make          # Build library only
make test     # Build and run tests
make clean    # Clean build artifacts
make help     # Show available targets
```

## What's included

The SDK vendors a small MD5 implementation (picohash) in `third_party/` for non-cryptographic sharding purposes. This eliminates the need for external dependencies like OpenSSL.

## Targets

- `make` or `make all` - Build the static library (`build/libeppoclient.a`)
- `make build` - Build the library with IDE support (generates `compile_commands.json`)
- `make test` - Build and run all tests
- `make clean` - Remove build artifacts
- `make help` - Show available targets

## Cross-Platform Compatibility

The build system works identically on:
- Linux (any distribution)
- macOS (with or without Homebrew)
- Other UNIX-like systems with a C++17 compiler

No special configuration or system dependencies required!

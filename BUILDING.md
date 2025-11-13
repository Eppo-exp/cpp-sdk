# Building the C++ SDK

## Requirements

- C++17 compatible compiler (g++ or clang++)
- C99 compatible compiler (gcc or clang)
- Make or CMake 3.14+

**No external dependencies required!** All hashing is done using vendored libraries in `third_party/`.

## Building with Make

```bash
make          # Build library only
make test     # Build and run tests
make clean    # Clean build artifacts
make help     # Show available targets
```

## Building with CMake

```bash
# Build library
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Build with examples
cmake -B build -DCMAKE_BUILD_TYPE=Release -DEPPOCLIENT_BUILD_EXAMPLES=ON
cmake --build build

# Build with tests
cmake -B build -DCMAKE_BUILD_TYPE=Release -DEPPOCLIENT_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build

# Install (optional)
sudo cmake --install build
```

### Building for Specific Architectures

```bash
# macOS: Build for Apple Silicon
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build

# macOS: Build for Intel
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=x86_64
cmake --build build

# macOS: Build Universal Binary (both architectures)
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
cmake --build build

# Windows: Build for ARM64
cmake -B build -DCMAKE_BUILD_TYPE=Release -A ARM64
cmake --build build --config Release

# Linux: Cross-compile for ARM64
CC=aarch64-linux-gnu-gcc CXX=aarch64-linux-gnu-g++ \
cmake -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_PROCESSOR=aarch64
cmake --build build
```

For more detailed architecture support and cross-compilation instructions, see [ARCHITECTURE.md](ARCHITECTURE.md).

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

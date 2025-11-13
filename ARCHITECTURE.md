# Supported Architectures

The Eppo C++ SDK supports multiple platforms and architectures. Pre-built binaries are available for common platforms, or you can build from source for any platform with a C++17 compiler.

## Pre-built Binaries

When you create a release, GitHub Actions automatically builds binaries for:

### Linux
- **x86_64** (Intel/AMD 64-bit) - Most common Linux servers and desktops
- **aarch64** (ARM64) - ARM-based servers (AWS Graviton, cloud ARM instances)

### macOS
- **x86_64** (Intel Macs)
- **arm64** (Apple Silicon - M1, M2, M3, M4)
- **Universal** (Fat binary containing both x86_64 and arm64)

### Windows
- **x86_64** (64-bit Windows on Intel/AMD)
- **arm64** (Windows on ARM - Surface Pro X, etc.)

## Using Pre-built Binaries

Download the appropriate binary for your platform from the [releases page](https://github.com/Eppo-exp/cpp-sdk/releases):

```bash
# Example: Linux x86_64
wget https://github.com/Eppo-exp/cpp-sdk/releases/download/v1.0.0/eppoclient-1.0.0-linux-x86_64.tar.gz
tar -xzf eppoclient-1.0.0-linux-x86_64.tar.gz

# Headers are in: include/
# Library is in: lib/libeppoclient.a
```

Then link against it:

```bash
g++ -std=c++17 your_app.cpp -I./include -L./lib -leppoclient -o your_app
```

## Building from Source

### Native Build

Build for your current architecture:

```bash
# Using Make
make

# Using CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Cross-Compilation

#### Linux: Cross-compile for ARM64 from x86_64

```bash
# Install cross-compiler
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# Build with CMake
CC=aarch64-linux-gnu-gcc CXX=aarch64-linux-gnu-g++ \
cmake -B build-arm64 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_SYSTEM_PROCESSOR=aarch64
cmake --build build-arm64
```

#### macOS: Build for Specific Architecture

```bash
# Build for Apple Silicon (arm64)
cmake -B build-arm64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build-arm64

# Build for Intel (x86_64)
cmake -B build-x86 -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=x86_64
cmake --build build-x86
```

#### macOS: Build Universal Binary

Create a "fat" binary that runs on both Intel and Apple Silicon:

```bash
# Build both architectures
cmake -B build-x86 -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=x86_64
cmake --build build-x86

cmake -B build-arm64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build-arm64

# Combine into universal binary
mkdir -p build-universal
lipo -create \
  build-x86/libeppoclient.a \
  build-arm64/libeppoclient.a \
  -output build-universal/libeppoclient.a

# Verify
lipo -info build-universal/libeppoclient.a
# Output: Architectures in the fat file: build-universal/libeppoclient.a are: x86_64 arm64
```

#### Windows: Build for ARM64

```bash
# Using CMake with MSVC
cmake -B build-arm64 -DCMAKE_BUILD_TYPE=Release -A ARM64
cmake --build build-arm64 --config Release
```

## Additional Architectures

While pre-built binaries are provided for the most common architectures, the SDK can be built for any platform with:
- C++17 compatible compiler
- C99 compatible compiler (for vendored MD5 implementation)

### Building for Other Architectures

The SDK has minimal dependencies and should compile on:
- **ARMv7** (32-bit ARM - Raspberry Pi, embedded devices)
- **RISC-V** (with appropriate toolchain)
- **PowerPC** (with appropriate toolchain)
- **s390x** (IBM mainframes)

Example for ARMv7:

```bash
# Install ARM cross-compiler
sudo apt-get install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf

# Build
CC=arm-linux-gnueabihf-gcc CXX=arm-linux-gnueabihf-g++ \
cmake -B build-armv7 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_SYSTEM_PROCESSOR=armv7
cmake --build build-armv7
```

## Docker Cross-Compilation

For more complex cross-compilation scenarios, use Docker:

```bash
# Example: Build for Linux ARM64 using Docker
docker run --rm -v $(pwd):/workspace -w /workspace \
  arm64v8/ubuntu:22.04 \
  bash -c "apt-get update && apt-get install -y build-essential cmake && \
           cmake -B build -DCMAKE_BUILD_TYPE=Release && \
           cmake --build build"
```

## Verifying Build Architecture

### Linux

```bash
file build/libeppoclient.a
# Output example: build/libeppoclient.a: current ar archive
readelf -h build/libeppoclient.a | grep Machine
# Output: Machine: AArch64 (or x86-64)
```

### macOS

```bash
lipo -info build/libeppoclient.a
# Output: Non-fat file: build/libeppoclient.a is architecture: arm64
# Or: Architectures in the fat file: ... are: x86_64 arm64
```

### Windows

```powershell
dumpbin /headers build\Release\eppoclient.lib
# Look for "machine" in the output
```

## Choosing the Right Binary

- **Linux servers (AWS, GCP, Azure)**: Use `linux-x86_64` unless using ARM instances (Graviton)
- **macOS developers**: Use `macos-universal` for maximum compatibility
- **macOS production**: Use architecture-specific binary for smaller size
- **Windows**: Use `windows-x86_64` for most PCs, `windows-arm64` for Surface Pro X and similar
- **Embedded/IoT**: Build from source for your specific architecture

## Performance Considerations

- **Native builds** always perform best - compile for your target architecture when possible
- **Universal binaries** (macOS) are slightly larger but more convenient
- **Cross-compiled binaries** have identical performance to native builds
- The SDK has no assembly code, so it's fully portable across architectures

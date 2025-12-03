# Release Process

This document describes how to create a new release of the Eppo C++ SDK.

## Version Numbering

We use [Semantic Versioning](https://semver.org/):
- **MAJOR**: Incompatible API changes
- **MINOR**: Backward-compatible functionality additions
- **PATCH**: Backward-compatible bug fixes

## Release Checklist

Before creating a release, ensure:

- [ ] All tests pass (`make test`)
- [ ] Examples compile and run correctly
- [ ] Documentation is up to date
- [ ] CHANGELOG has been updated
- [ ] Version is updated in `src/version.hpp`

## Creating a Release

### 1. Update Version

Edit `src/version.hpp` and update the version numbers:

```cpp
#define EPPOCLIENT_VERSION_MAJOR 1
#define EPPOCLIENT_VERSION_MINOR 1
#define EPPOCLIENT_VERSION_PATCH 0
#define EPPOCLIENT_VERSION "1.1.0"
```

### 2. Commit Version Changes

```bash
git add src/version.hpp
git commit -m "chore: bump version to v1.1.0"
## Merge PR after tests pass
```

### 3. Create and Push Tag

```bash
git fetch origin main && git reset --hard origin/main
git tag -a v1.1.0 -m "v1.1.0"
git push origin HEAD --tags
```

### 4. Create GitHub Release

Create a new release on GitHub:

1. Go to the [Releases page](https://github.com/Eppo-exp/cpp-sdk/releases)
2. Click "Draft a new release"
3. Choose the tag you just pushed (v1.1.0)
4. Click "Generate release notes"
6. Click "Publish release"

### 5. Automated Binary Builds

Once you publish the release, the GitHub Actions workflow (`.github/workflows/publish.yml`) will automatically:
- Build the library for multiple architectures:
  - **Linux**: x86_64, aarch64
  - **macOS**: x86_64, arm64, Universal Binary
  - **Windows**: x86_64, arm64
- Upload pre-built binaries as release assets (7 total binaries)
- This process takes approximately 15-20 minutes

You can monitor the build progress in the [Actions tab](https://github.com/Eppo-exp/cpp-sdk/actions).

## Testing a Release Locally

Before pushing a tag, you can test the release build process:

```bash
# Test Make build
make clean && make

# Test CMake build
rm -rf build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Test examples
cmake -B build -DEPPOCLIENT_BUILD_EXAMPLES=ON
cmake --build build
cd examples && ../build/bandits

# Test installation
cmake --install build --prefix /tmp/test-install
```

## Troubleshooting

### Release workflow failed
Check the [Actions tab](https://github.com/Eppo-exp/cpp-sdk/actions) for error details.

### Need to delete a release
```bash
# Delete the tag locally
git tag -d v1.0.0

# Delete the tag on GitHub
git push origin :refs/tags/v1.0.0

# Then delete the release from the GitHub UI
```

### Need to re-create a release
1. Delete the release and tag (see above)
2. Fix any issues
3. Create a new commit with fixes
4. Create and push the tag again

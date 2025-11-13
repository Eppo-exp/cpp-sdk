#!/bin/bash
# Verify that EPPOCLIENT_VERSION matches the component versions
# (EPPOCLIENT_VERSION_MAJOR.MINOR.PATCH)
# Treats EPPOCLIENT_VERSION as the source of truth.

set -e

VERSION_FILE="${1:-src/version.hpp}"

if [ ! -f "$VERSION_FILE" ]; then
  echo "❌ ERROR: Version file not found: $VERSION_FILE"
  exit 1
fi

# Extract version components from version.hpp
MAJOR=$(grep '#define EPPOCLIENT_VERSION_MAJOR' "$VERSION_FILE" | awk '{print $3}')
MINOR=$(grep '#define EPPOCLIENT_VERSION_MINOR' "$VERSION_FILE" | awk '{print $3}')
PATCH=$(grep '#define EPPOCLIENT_VERSION_PATCH' "$VERSION_FILE" | awk '{print $3}')
VERSION=$(grep '#define EPPOCLIENT_VERSION ' "$VERSION_FILE" | awk '{print $3}' | tr -d '"')

# Parse VERSION string into expected components
IFS='.' read -r EXPECTED_MAJOR EXPECTED_MINOR EXPECTED_PATCH <<< "$VERSION"

echo "EPPOCLIENT_VERSION: $VERSION"
echo "Component versions: MAJOR=$MAJOR, MINOR=$MINOR, PATCH=$PATCH"

# Check for mismatches
MISMATCHES=()
if [ "$MAJOR" != "$EXPECTED_MAJOR" ]; then
  MISMATCHES+=("EPPOCLIENT_VERSION_MAJOR is $MAJOR but should be $EXPECTED_MAJOR")
fi
if [ "$MINOR" != "$EXPECTED_MINOR" ]; then
  MISMATCHES+=("EPPOCLIENT_VERSION_MINOR is $MINOR but should be $EXPECTED_MINOR")
fi
if [ "$PATCH" != "$EXPECTED_PATCH" ]; then
  MISMATCHES+=("EPPOCLIENT_VERSION_PATCH is $PATCH but should be $EXPECTED_PATCH")
fi

# Report results
if [ ${#MISMATCHES[@]} -gt 0 ]; then
  echo ""
  echo "❌ ERROR: Version component mismatch!"
  for mismatch in "${MISMATCHES[@]}"; do
    echo "  - $mismatch"
  done
  echo ""
  echo "Please update the component version(s) in $VERSION_FILE to match EPPOCLIENT_VERSION."
  exit 1
fi

echo "✅ Version consistency check passed!"

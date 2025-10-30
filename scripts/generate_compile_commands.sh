#!/bin/bash
# Generate compile_commands.json for clangd LSP support
# This script can be run from anywhere

# Get the absolute path of the project root (parent of scripts directory)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Compiler settings from Makefile
CXX="g++"
CXXFLAGS="-std=c++17 -Wall -Wextra -I. -Ithird_party"
BUILD_DIR="build"

# Automatically find all source files
# Collect .cpp and .cc files from src/ and test/ directories
# Note: third_party files are excluded - headers are available via CXXFLAGS include paths
SOURCE_FILES=()

# Find source files in src/ directory
if [ -d "${PROJECT_ROOT}/src" ]; then
    while IFS= read -r -d '' file; do
        # Make path relative to project root
        relative_path="${file#${PROJECT_ROOT}/}"
        SOURCE_FILES+=("${relative_path}")
    done < <(find "${PROJECT_ROOT}/src" -type f \( -name "*.cpp" -o -name "*.cc" \) -print0 | sort -z)
fi

# Find test files in test/ directory
if [ -d "${PROJECT_ROOT}/test" ]; then
    while IFS= read -r -d '' file; do
        relative_path="${file#${PROJECT_ROOT}/}"
        SOURCE_FILES+=("${relative_path}")
    done < <(find "${PROJECT_ROOT}/test" -type f \( -name "*.cpp" -o -name "*.cc" \) -print0 | sort -z)
fi

# Check if we found any source files
if [ ${#SOURCE_FILES[@]} -eq 0 ]; then
    echo "Warning: No source files found in src/ or test/ directories"
    exit 1
fi

echo "Found ${#SOURCE_FILES[@]} source file(s)"

# Generate compile_commands.json
cat > "${PROJECT_ROOT}/compile_commands.json" << EOF
[
EOF

# Add entries for each source file
for i in "${!SOURCE_FILES[@]}"; do
    SOURCE_FILE="${SOURCE_FILES[$i]}"
    OBJECT_FILE="${BUILD_DIR}/$(basename ${SOURCE_FILE%.cpp}.o)"

    # Add comma before entry (except for the first one)
    if [ $i -gt 0 ]; then
        echo "," >> "${PROJECT_ROOT}/compile_commands.json"
    fi

    cat >> "${PROJECT_ROOT}/compile_commands.json" << EOF
  {
    "directory": "${PROJECT_ROOT}",
    "command": "${CXX} ${CXXFLAGS} -c ${SOURCE_FILE} -o ${OBJECT_FILE}",
    "file": "${SOURCE_FILE}"
  }
EOF
done

# Close the JSON array
cat >> "${PROJECT_ROOT}/compile_commands.json" << EOF

]
EOF

echo "Generated compile_commands.json at ${PROJECT_ROOT}/compile_commands.json"



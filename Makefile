# Compiler and flags
CXX = g++
CC = gcc
CXXFLAGS = -std=c++17 -Wall -Wextra -I. -Ithird_party
CFLAGS = -std=c99 -Wall -Wextra -I. -Ithird_party
LDFLAGS =

# Directories
SRC_DIR = src
TEST_DIR = test
BUILD_DIR = build

# Source files
LIB_SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
LIB_OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(LIB_SOURCES))

# Third party files (dynamically discovered)
THIRD_PARTY_CPP_SOURCES = $(wildcard third_party/*.cpp)
THIRD_PARTY_C_SOURCES = $(wildcard third_party/*.c)
THIRD_PARTY_OBJECTS = $(patsubst third_party/%.cpp,$(BUILD_DIR)/%.o,$(THIRD_PARTY_CPP_SOURCES)) \
                      $(patsubst third_party/%.c,$(BUILD_DIR)/%.o,$(THIRD_PARTY_C_SOURCES))

# Auto-discover all test files
TEST_SOURCES = $(wildcard $(TEST_DIR)/test_*.cpp)
TEST_EXECUTABLE = $(BUILD_DIR)/test_runner

# Library output
LIBRARY = $(BUILD_DIR)/libeppoclient.a

# Build library object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Pattern rule for third_party C++ files
$(BUILD_DIR)/%.o: third_party/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Pattern rule for third_party C files
$(BUILD_DIR)/%.o: third_party/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create static library
$(LIBRARY): $(LIB_OBJECTS) $(THIRD_PARTY_OBJECTS)
	ar rcs $@ $^
	@echo "Library built: $(LIBRARY)"

# Build test runner executable
$(TEST_EXECUTABLE): $(LIBRARY) $(TEST_SOURCES) $(THIRD_PARTY_OBJECTS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(TEST_SOURCES) $(THIRD_PARTY_OBJECTS) $(LIBRARY) $(LDFLAGS) -o $@
	@echo "Test runner built: $(TEST_EXECUTABLE)"

# Default target
.PHONY: all
all: $(LIBRARY)

# Build/compile targets for checking compilation errors
.PHONY: build compile
build: 
	@echo "Generating compile_commands.json for IDE support..."
	rm -f compile_commands.json
	./scripts/generate_compile_commands.sh
	@mkdir -p $(BUILD_DIR)
	@$(MAKE) all

## test-data
branchName := main
.PHONY: test-data
test-data:
	rm -rf test/data
	git clone -b ${branchName} --depth 1 --single-branch https://github.com/Eppo-exp/sdk-test-data.git test/data/

# Run all tests (primary test target)
.PHONY: test
test: test-data
	@$(MAKE) $(TEST_EXECUTABLE)
	@echo "Running all tests..."
	@./$(TEST_EXECUTABLE)

# Clean build artifacts
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	@echo "Build directory cleaned"

# Help target
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all              - Build the library (default)"
	@echo "  build            - Build and configure IDE support"
	@echo "  test             - Build and run all tests"
	@echo "  clean            - Remove build artifacts"
	@echo "  help             - Show this help message"

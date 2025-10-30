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

# MD5 wrapper (C code)
MD5_WRAPPER = $(BUILD_DIR)/md5_wrapper.o

# Auto-discover all test files
TEST_SOURCES = $(wildcard $(TEST_DIR)/test_*.cpp)
TEST_EXECUTABLE = $(BUILD_DIR)/test_runner

# Catch2 test framework
CATCH_OBJECTS = $(BUILD_DIR)/catch_amalgamated.o

# Library output
LIBRARY = $(BUILD_DIR)/libeppoclient.a

# Default target
.PHONY: all
all: $(LIBRARY)

# Build/compile targets for checking compilation errors
.PHONY: build compile
build: clean
	@echo "Generating compile_commands.json for IDE support..."
	./scripts/generate_compile_commands.sh
	@mkdir -p $(BUILD_DIR)
	@$(MAKE) all

# Build library object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Build MD5 wrapper (C code)
$(BUILD_DIR)/md5_wrapper.o: third_party/md5_wrapper.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create static library
$(LIBRARY): $(LIB_OBJECTS) $(MD5_WRAPPER)
	ar rcs $@ $^
	@echo "Library built: $(LIBRARY)"

# Build Catch2 framework
$(BUILD_DIR)/catch_amalgamated.o: third_party/catch_amalgamated.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Build test runner executable
$(TEST_EXECUTABLE): $(LIBRARY) $(TEST_SOURCES) $(CATCH_OBJECTS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(TEST_SOURCES) $(CATCH_OBJECTS) $(LIBRARY) $(LDFLAGS) -o $@
	@echo "Test runner built: $(TEST_EXECUTABLE)"

# Run all tests (primary test target)
.PHONY: test
test: build
	@$(MAKE) $(TEST_EXECUTABLE)
	@echo "Running all tests..."
	@./$(TEST_EXECUTABLE)

# Clean build artifacts
.PHONY: clean
clean:
	rm -f compile_commands.json
	rm -rf $(BUILD_DIR)
	@echo "Build directory cleaned"

# Help target
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all              - Build the library (default)"
	@echo "  build            - Clean-build and configure IDE support"
	@echo "  test             - Clean-build and run all tests"
	@echo "  clean            - Remove build artifacts"
	@echo "  help             - Show this help message"

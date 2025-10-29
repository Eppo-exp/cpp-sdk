# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -I.

# Directories
SRC_DIR = src
TEST_DIR = test
BUILD_DIR = build

# Source files
LIB_SOURCES = $(SRC_DIR)/flag_sdk.cpp
LIB_OBJECTS = $(BUILD_DIR)/flag_sdk.o
TEST_SOURCES = $(TEST_DIR)/test_flag_sdk.cpp
TEST_EXECUTABLE = $(BUILD_DIR)/test_runner

# Library output
LIBRARY = $(BUILD_DIR)/libflagsdk.a

# Default target
.PHONY: all
all: $(LIBRARY)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build library object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Create static library
$(LIBRARY): $(LIB_OBJECTS)
	ar rcs $@ $^
	@echo "Library built: $(LIBRARY)"

# Build and run tests
.PHONY: test
test: $(TEST_EXECUTABLE)
	@echo "Running tests..."
	@./$(TEST_EXECUTABLE)

# Build test executable
$(TEST_EXECUTABLE): $(TEST_SOURCES) $(LIB_OBJECTS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(TEST_SOURCES) $(LIB_OBJECTS) -o $@

# Clean build artifacts
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	@echo "Build directory cleaned"

# Help target
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all    - Build the library (default)"
	@echo "  test   - Build and run tests"
	@echo "  clean  - Remove build artifacts"
	@echo "  help   - Show this help message"

# Compiler and flags
CXX = g++
CC = gcc
CXXFLAGS = -std=c++17 -Wall -Wextra -I. -Ithird_party -MMD -MP
CFLAGS = -std=c99 -Wall -Wextra -I. -Ithird_party -MMD -MP
LDFLAGS =

# Directories
SRC_DIR = src
TEST_DIR = test
BUILD_DIR = build
EXAMPLES_DIR = examples

# Source files
LIB_SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
LIB_OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(LIB_SOURCES))

# Third party files (dynamically discovered)
THIRD_PARTY_CPP_SOURCES = $(wildcard third_party/*.cpp)
THIRD_PARTY_C_SOURCES = $(wildcard third_party/*.c)
THIRD_PARTY_OBJECTS = $(patsubst third_party/%.cpp,$(BUILD_DIR)/%.o,$(THIRD_PARTY_CPP_SOURCES)) \
                      $(patsubst third_party/%.c,$(BUILD_DIR)/%.o,$(THIRD_PARTY_C_SOURCES))

# Auto-discover all test files
TEST_SOURCES = $(wildcard $(TEST_DIR)/test_*.cpp) $(wildcard $(TEST_DIR)/shared_test_cases/test_*.cpp)
TEST_EXECUTABLE = $(BUILD_DIR)/test_runner

# Library output
LIBRARY = $(BUILD_DIR)/libeppoclient.a

# Dependency files
DEPFILES = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.d,$(LIB_SOURCES)) \
           $(patsubst third_party/%.cpp,$(BUILD_DIR)/%.d,$(THIRD_PARTY_CPP_SOURCES)) \
           $(patsubst third_party/%.c,$(BUILD_DIR)/%.d,$(THIRD_PARTY_C_SOURCES))

# Include dependency files if they exist
-include $(DEPFILES)

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
	rm -f compile_commands.json
	./scripts/generate_compile_commands.sh
	@$(MAKE) $(TEST_EXECUTABLE)
	@echo "Running all tests..."
	@./$(TEST_EXECUTABLE)

# Clean build artifacts
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	rm -f compile_commands.json
	@echo "Build directory cleaned"

# Example executables
EXAMPLE_BANDITS = $(BUILD_DIR)/bandits
EXAMPLE_FLAGS = $(BUILD_DIR)/flag_assignments
EXAMPLE_ASSIGNMENT_DETAILS = $(BUILD_DIR)/assignment_details

# Build example executables
$(EXAMPLE_BANDITS): $(LIBRARY) $(EXAMPLES_DIR)/bandits.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(EXAMPLES_DIR)/bandits.cpp $(LIBRARY) $(LDFLAGS) -o $@
	@echo "Example built: $(EXAMPLE_BANDITS)"

$(EXAMPLE_FLAGS): $(LIBRARY) $(EXAMPLES_DIR)/flag_assignments.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(EXAMPLES_DIR)/flag_assignments.cpp $(LIBRARY) $(LDFLAGS) -o $@
	@echo "Example built: $(EXAMPLE_FLAGS)"

$(EXAMPLE_ASSIGNMENT_DETAILS): $(LIBRARY) $(EXAMPLES_DIR)/assignment_details.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(EXAMPLES_DIR)/assignment_details.cpp $(LIBRARY) $(LDFLAGS) -o $@
	@echo "Example built: $(EXAMPLE_ASSIGNMENT_DETAILS)"

# Build all examples
.PHONY: examples
examples: $(EXAMPLE_BANDITS) $(EXAMPLE_FLAGS)
	@echo "All examples built successfully"

# Run specific examples (must be run from examples directory for config paths)
.PHONY: run-bandits
run-bandits: $(EXAMPLE_BANDITS)
	@echo "Running bandits example..."
	@cd $(EXAMPLES_DIR) && ../$(EXAMPLE_BANDITS)

.PHONY: run-flags
run-flags: $(EXAMPLE_FLAGS)
	@echo "Running flag_assignments example..."
	@cd $(EXAMPLES_DIR) && ../$(EXAMPLE_FLAGS)

.PHONY: run-assignment-details
run-assignment-details: $(EXAMPLE_ASSIGNMENT_DETAILS)
	@echo "Running assignment_details example..."
	@cd $(EXAMPLES_DIR) && ../$(EXAMPLE_ASSIGNMENT_DETAILS)

# Interactive example runner
.PHONY: run-example
run-example:
	@echo "Available examples:"
	@echo "  1) bandits           - Demonstrates bandit functionality"
	@echo "  2) flag_assignments  - Demonstrates flag assignment functionality"
	@echo ""
	@echo "Usage:"
	@echo "  make run-bandits            - Run the bandits example"
	@echo "  make run-flags              - Run the flag assignments example"
	@echo "  make run-assignment-details - Run the assignment details example"
	@echo ""
	@echo "Or specify EXAMPLE variable:"
	@echo "  make run-example EXAMPLE=bandits"
	@echo "  make run-example EXAMPLE=flags"
	@echo "  make run-example EXAMPLE=assignment-details"
	@if [ -n "$(EXAMPLE)" ]; then \
		if [ "$(EXAMPLE)" = "bandits" ]; then \
			$(MAKE) run-bandits; \
		elif [ "$(EXAMPLE)" = "flags" ]; then \
			$(MAKE) run-flags; \
		elif [ "$(EXAMPLE)" = "assignment-details" ]; then \
			$(MAKE) run-assignment-details; \
		else \
			echo "Error: Unknown example '$(EXAMPLE)'"; \
			echo "Valid options: bandits, flags, assignment-details"; \
			exit 1; \
		fi \
	fi

# Help target
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all                    - Build the library (default)"
	@echo "  build                  - Build and configure IDE support"
	@echo "  test                   - Build and run all tests"
	@echo "  examples               - Build all examples"
	@echo "  run-example            - Show available examples and usage"
	@echo "  run-bandits            - Run the bandits example"
	@echo "  run-flags              - Run the flag assignments example"
	@echo "  run-assignment-details - Run the assignment details example"
	@echo "  clean                  - Remove build artifacts"
	@echo "  help                   - Show this help message"

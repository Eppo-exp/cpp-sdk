BUILD_DIR = build

# Test data branch (override with: make test TEST_DATA_BRANCH=my-branch)
TEST_DATA_BRANCH ?= main

# Default target - comprehensive development build (library + tests + examples)
.PHONY: all
all:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DEPPOCLIENT_BUILD_TESTS=ON \
		-DEPPOCLIENT_BUILD_EXAMPLES=ON \
		-DEPPOCLIENT_ERR_ON_WARNINGS=ON \
		-DTEST_DATA_BRANCH=$(TEST_DATA_BRANCH)
	@cd $(BUILD_DIR) && $(MAKE)
	@# Copy compile_commands.json to root for IDE support
	@if [ -f $(BUILD_DIR)/compile_commands.json ]; then \
		cp $(BUILD_DIR)/compile_commands.json .; \
		echo "Updated compile_commands.json for IDE support"; \
	fi
	@echo "Build complete: library, tests, and examples"
	@echo "  Library: $(BUILD_DIR)/libeppoclient.a"
	@echo "  Tests:   $(BUILD_DIR)/test_runner"
	@echo "  Run tests with: make test"

# Build library only (production-friendly, no warnings-as-errors)
.PHONY: build
build:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DEPPOCLIENT_BUILD_TESTS=OFF \
		-DEPPOCLIENT_ERR_ON_WARNINGS=OFF
	@cd $(BUILD_DIR) && $(MAKE)
	@# Copy compile_commands.json to root for IDE support
	@if [ -f $(BUILD_DIR)/compile_commands.json ]; then \
		cp $(BUILD_DIR)/compile_commands.json .; \
	fi
	@echo "Library built: $(BUILD_DIR)/libeppoclient.a"

# Run all tests (primary test target)
.PHONY: test
test:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DEPPOCLIENT_BUILD_TESTS=ON \
		-DEPPOCLIENT_ERR_ON_WARNINGS=ON \
		-DTEST_DATA_BRANCH=$(TEST_DATA_BRANCH)
	@cd $(BUILD_DIR) && $(MAKE)
	@# Copy compile_commands.json to root for IDE support
	@if [ -f $(BUILD_DIR)/compile_commands.json ]; then \
		cp $(BUILD_DIR)/compile_commands.json .; \
		echo "Updated compile_commands.json for IDE support"; \
	fi
	@echo "Running tests..."
	@cd $(BUILD_DIR) && ctest -V

# Build all examples
.PHONY: examples
examples:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DEPPOCLIENT_BUILD_EXAMPLES=ON \
		-DEPPOCLIENT_ERR_ON_WARNINGS=OFF
	@cd $(BUILD_DIR) && $(MAKE)
	@echo "Examples built in $(BUILD_DIR)/"
	@echo ""
	@echo "Run examples with:"
	@echo "  make run-bandits"
	@echo "  make run-flags"
	@echo "  make run-assignment-details"

# Run example binaries
.PHONY: run-bandits
run-bandits: examples
	@echo "Running bandits example..."
	@$(BUILD_DIR)/bandits

.PHONY: run-flags
run-flags: examples
	@echo "Running flag_assignments example..."
	@$(BUILD_DIR)/flag_assignments

.PHONY: run-assignment-details
run-assignment-details: examples
	@echo "Running assignment_details example..."
	@$(BUILD_DIR)/assignment_details

# Clean build artifacts
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	rm -f compile_commands.json
	@echo "Build directory cleaned"

# Memory testing with sanitizers
.PHONY: test-memory
test-memory:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DEPPOCLIENT_BUILD_TESTS=ON \
		-DEPPOCLIENT_ERR_ON_WARNINGS=ON \
		-DTEST_DATA_BRANCH=$(TEST_DATA_BRANCH) \
		-DCMAKE_BUILD_TYPE=Debug \
		-DEPPOCLIENT_SANITIZE=ON
	@cd $(BUILD_DIR) && $(MAKE)
	@echo "Running tests with AddressSanitizer and UndefinedBehaviorSanitizer..."
	@env ASAN_OPTIONS=halt_on_error=1:strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:print_stats=1 \
		UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
		./build/test_runner "[ufc]"

# Performance testing
.PHONY: test-eval-performance
test-eval-performance:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DEPPOCLIENT_BUILD_TESTS=ON \
		-DEPPOCLIENT_ERR_ON_WARNINGS=ON \
		-DTEST_DATA_BRANCH=$(TEST_DATA_BRANCH) \
		-DCMAKE_BUILD_TYPE=RelWithDebInfo
	@cd $(BUILD_DIR) && $(MAKE)
	@echo "Running performance tests..."
	@./build/test_runner "[performance]"

# Format all source files with clang-format
.PHONY: format
format:
	@echo "Formatting C++ source files..."
	@find src test examples -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) -exec clang-format -i {} +
	@echo "Formatting complete!"

# Check formatting without modifying files
.PHONY: format-check
format-check:
	@echo "Checking C++ source formatting..."
	@find src test examples -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) -exec clang-format --dry-run -Werror {} + && echo "All files are properly formatted!" || (echo "ERROR: Some files need formatting. Run 'make format' to fix."; exit 1)

# Help
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all (default)          - Build everything: library + tests + examples (with -Werror)"
	@echo "  build                  - Build library only (production-friendly, no -Werror)"
	@echo "  test                   - Build and run all tests (with -Werror)"
	@echo "  test-memory            - Run tests with AddressSanitizer and UndefinedBehaviorSanitizer"
	@echo "  test-eval-performance  - Run flag evaluation performance tests (min/max/avg μs)"
	@echo "  examples               - Build all examples"
	@echo "  run-bandits            - Build and run the bandits example"
	@echo "  run-flag-assignments   - Build and run the flag_assignments example"
	@echo "  run-assignment-details - Build and run the assignment_details example"
	@echo "  format                 - Format all C++ source files with clang-format"
	@echo "  format-check           - Check if files are properly formatted (CI-friendly)"
	@echo "  clean                  - Remove build artifacts"
	@echo "  help                   - Show this help message"
	@echo ""
	@echo "Options:"
	@echo "  TEST_DATA_BRANCH       - Branch of sdk-test-data to fetch (default: main)"
	@echo "                           Example: make test TEST_DATA_BRANCH=feature-branch"


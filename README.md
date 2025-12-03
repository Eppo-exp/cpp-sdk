# Eppo C++ SDK

[Eppo](https://geteppo.com) is a modular flagging and experimentation analysis tool.

## Features

The Eppo C++ SDK is designed for offline flag evaluation and supports:

- **Feature Flags**: Boolean, numeric, integer, string, and JSON flag evaluations
- **Contextual Bandits**: ML-powered dynamic variant selection
- **Assignment Logging**: Track which variants are assigned to users, with automatic deduplication
- **Contextual Bandit Logging**: Track bandit actions, with automatic deduplication
- **Application Logging**: Debug and monitor SDK behavior
- **No Exceptions**: Built with `-fno-exceptions` for compatibility with exception-free projects

## Installation

### Using CMake FetchContent (Recommended)

The easiest way to integrate the Eppo C++ SDK is using CMake's FetchContent:

```cmake
include(FetchContent)
FetchContent_Declare(
  eppoclient
  GIT_REPOSITORY https://github.com/Eppo-exp/cpp-sdk.git
  GIT_TAG v2.0.0  # Use the latest version
)
FetchContent_MakeAvailable(eppoclient)

# Link against your target
target_link_libraries(your_application PRIVATE eppoclient)
```

### Using CMake with Local Installation

If you prefer to install the library system-wide:

```bash
# Clone the repository
git clone https://github.com/Eppo-exp/cpp-sdk.git
cd cpp-sdk

# Build and install
## (see ARCHITECTURE.md for building with specific architectures)
cmake -B build -DCMAKE_BUILD_TYPE=Release 
cmake --build build
sudo cmake --install build

# In your CMakeLists.txt:
find_package(eppoclient REQUIRED)
target_link_libraries(your_application PRIVATE eppoclient::eppoclient)
```

### Using Make (Manual Integration)

For projects not using CMake, you can build the static library with Make:

```bash
# Clone and build
git clone https://github.com/Eppo-exp/cpp-sdk.git
cd cpp-sdk
make

# This creates build/libeppoclient.a
# Link against it in your project:
g++ -std=c++17 your_app.cpp -I/path/to/cpp-sdk/src -I/path/to/cpp-sdk/third_party \
    -L/path/to/cpp-sdk/build -leppoclient -o your_app
```

### Download Pre-built Binaries

Download pre-built libraries for your platform from the [releases page](https://github.com/Eppo-exp/cpp-sdk/releases).

**Available platforms:**
- Linux: x86_64, aarch64 (ARM64)
- macOS: x86_64 (Intel), arm64 (Apple Silicon), Universal
- Windows: x86_64, arm64

See [ARCHITECTURE.md](ARCHITECTURE.md) for detailed architecture support and cross-compilation instructions.

### Requirements

- C++17 compatible compiler (g++, clang++, or MSVC)
- CMake 3.14+ (if using CMake)
- Make (if using Make)
- RE2 - Google's safe regex library
- 64-bit architecture (x86_64, aarch64/ARM64)

#### Installing RE2

**Ubuntu/Debian:**
```bash
sudo apt-get install libre2-dev
```

**macOS (Homebrew):**
```bash
brew install re2
```

**Fedora/RHEL:**
```bash
sudo dnf install re2-devel
```

**From source:**
```bash
git clone https://github.com/google/re2.git
cd re2
mkdir build && cd build
cmake .. && make
sudo make install
```

Other dependencies (nlohmann/json, semver, etc.) are vendored and require no installation.

**Note:** Only 64-bit architectures are supported. 32-bit (x86) builds are not supported.

## Quick Start

### 1. Initialize the SDK

The Eppo SDK requires configuration data containing your feature flags. This SDK is designed for offline use, so you'll load configuration directly rather than using SDK keys or polling.

```cpp
#include "client.hpp"

// Your configuration as a JSON string
std::string configJson = R"({
    "flags": {
        "new-ui-rollout": {
            "enabled": true,
            "variations": {...}
        }
    }
})";

// Parse configuration from JSON string
auto result = eppoclient::parseConfiguration(configJson);
if (!result.hasValue()) {
    std::cerr << "Configuration parsing failed:" << std::endl;
    for (const auto& error : result.errors) {
        std::cerr << "  - " << error << std::endl;
    }
    return 1;
}

// Create and initialize the configuration store
auto configStore = std::make_shared<eppoclient::ConfigurationStore>();
configStore->setConfiguration(std::move(*result.value));

// Create the client
eppoclient::EppoClient client(configStore);
```

#### Alternative: Initialize ConfigurationStore with Configuration

You can also initialize the `ConfigurationStore` directly with a `Configuration` object using one of the convenience constructors:

```cpp
// Option 1: Pass Configuration by value
auto configStore = std::make_shared<eppoclient::ConfigurationStore>(std::move(*result.value));

// Option 2: Pass Configuration as shared_ptr
auto config = std::make_shared<const eppoclient::Configuration>(std::move(*result.value));
auto configStore = std::make_shared<eppoclient::ConfigurationStore>(config);

// Both options create a ConfigurationStore with the configuration already set
eppoclient::EppoClient client(configStore);
```

This is more concise than the two-step approach and is useful when you have your configuration ready at initialization time.

### 2. Evaluate Feature Flags

Once initialized, you can evaluate feature flags for different types:

```cpp
// Boolean flag
bool showNewFeature = client.getBooleanAssignment(
    "new-feature-flag",            // flag key
    "user-123",                    // subject key
    attributes,                    // subject attributes
    false                          // default value
);

// String flag
std::string buttonColor = client.getStringAssignment(
    "button-color",
    "user-123",
    attributes,
    "blue"
);

// Numeric flag
double discountRate = client.getNumericAssignment(
    "discount-rate",
    "user-123",
    attributes,
    0.0
);

// Integer flag
int64_t maxRetries = client.getIntegerAssignment(
    "max-retries",
    "user-123",
    attributes,
    3
);

// JSON flag
nlohmann::json config = client.getJSONAssignment(
    "feature-config",
    "user-123",
    attributes,
    nlohmann::json::object()
);

// Serialized JSON flag (returns stringified JSON)
std::string configString = client.getSerializedJSONAssignment(
    "feature-config",
    "user-123",
    attributes,
    "{}"  // default value as string
);
```

### 3. Add Assignment and Application Logging

To track assignments and monitor SDK behavior, implement custom loggers:

```cpp
// Assignment Logger - tracks which variants are assigned to users
class MyAssignmentLogger : public eppoclient::AssignmentLogger {
public:
    void logAssignment(const eppoclient::AssignmentEvent& event) override {
        // Send assignment data to your analytics platform
        std::cout << "Assignment: " << event.featureFlag
                  << " -> " << event.variation
                  << " for " << event.subject << std::endl;
    }
};

// Application Logger - logs SDK operational messages
class MyApplicationLogger : public eppoclient::ApplicationLogger {
public:
    void debug(const std::string& message) override {
        std::cout << "[DEBUG] " << message << std::endl;
    }

    void info(const std::string& message) override {
        std::cout << "[INFO] " << message << std::endl;
    }

    void warn(const std::string& message) override {
        std::cout << "[WARN] " << message << std::endl;
    }

    void error(const std::string& message) override {
        std::cerr << "[ERROR] " << message << std::endl;
    }
};

// Create client with loggers
auto assignmentLogger = std::make_shared<MyAssignmentLogger>();
auto applicationLogger = std::make_shared<MyApplicationLogger>();
auto configStore = std::make_shared<eppoclient::ConfigurationStore>();
// ... (after configStore->setConfiguration())

eppoclient::EppoClient client(
    configStore,
    assignmentLogger,
    nullptr,  // bandit logger (optional)
    applicationLogger
);
```

### Complete Example

Here's a complete example showing flag evaluation with logging:

```cpp
#include <iostream>
#include <memory>
#include "client.hpp"

int main() {
    // Initialize configuration
    std::string configJson = "...";  // Your JSON config string
    auto result = eppoclient::parseConfiguration(configJson);
    if (!result.hasValue()) {
        std::cerr << "Configuration parsing failed:" << std::endl;
        for (const auto& error : result.errors) {
            std::cerr << "  - " << error << std::endl;
        }
        return 1;
    }

    auto configStore = std::make_shared<eppoclient::ConfigurationStore>();
    configStore->setConfiguration(std::move(*result.value));

    // Create loggers
    auto assignmentLogger = std::make_shared<MyAssignmentLogger>();
    auto applicationLogger = std::make_shared<MyApplicationLogger>();

    // Create client
    eppoclient::EppoClient client(
        configStore,
        assignmentLogger,
        nullptr,
        applicationLogger
    );

    // Define subject attributes for targeting
    eppoclient::Attributes attributes;
    attributes["country"] = std::string("US");
    attributes["age"] = 25;
    attributes["is_premium"] = true;

    // Evaluate a feature flag
    bool showNewUI = client.getBooleanAssignment(
        "new-ui-rollout",
        "user-abc-123",
        attributes,
        false  // default value
    );

    if (showNewUI) {
        std::cout << "Showing new UI!" << std::endl;
        // Show new UI
    } else {
        std::cout << "Showing old UI" << std::endl;
        // Show old UI
    }

    return 0;
}
```

## Contextual Bandits

Eppo's contextual bandits allow you to dynamically select the best variant based on contextual attributes using machine learning models. This is useful for personalization, recommendation systems, and adaptive experiences.

### Setting Up Bandits

To use bandits, you need to load both flag configuration and bandit models:

```cpp
#include "client.hpp"

// Your configuration and bandit models as JSON strings
std::string flagConfigJson = "...";  // Your flag config JSON
std::string banditModelsJson = "...";  // Your bandit models JSON

auto result = eppoclient::parseConfiguration(flagConfigJson, banditModelsJson);
if (!result.hasValue()) {
    std::cerr << "Configuration parsing failed:" << std::endl;
    for (const auto& error : result.errors) {
        std::cerr << "  - " << error << std::endl;
    }
    return 1;
}

auto configStore = std::make_shared<eppoclient::ConfigurationStore>();
configStore->setConfiguration(std::move(*result.value));

// Create bandit logger to track bandit actions
class MyBanditLogger : public eppoclient::BanditLogger {
public:
    void logBanditAction(const eppoclient::BanditEvent& event) override {
        std::cout << "Bandit Action: " << event.action
                  << " (probability: " << event.actionProbability << ")"
                  << " for " << event.subject << std::endl;
    }
};

auto banditLogger = std::make_shared<MyBanditLogger>();

// Create client with bandit logger
eppoclient::EppoClient client(
    configStore,
    assignmentLogger,
    banditLogger,
    applicationLogger
);
```

### Getting Bandit Actions

Use `getBanditAction()` to get ML-powered recommendations:

```cpp
// Define subject attributes (user/context)
eppoclient::ContextAttributes subjectAttributes;
subjectAttributes.numericAttributes["age"] = 25.0;
subjectAttributes.categoricalAttributes["country"] = "US";
subjectAttributes.categoricalAttributes["device"] = "mobile";

// Define available actions with their attributes
std::map<std::string, eppoclient::ContextAttributes> actions;

// Action 1: Product A
eppoclient::ContextAttributes productA;
productA.numericAttributes["price"] = 29.99;
productA.numericAttributes["rating"] = 4.5;
productA.categoricalAttributes["category"] = "electronics";
actions["product-a"] = productA;

// Action 2: Product B
eppoclient::ContextAttributes productB;
productB.numericAttributes["price"] = 49.99;
productB.numericAttributes["rating"] = 4.8;
productB.categoricalAttributes["category"] = "electronics";
actions["product-b"] = productB;

// Action 3: Product C
eppoclient::ContextAttributes productC;
productC.numericAttributes["price"] = 19.99;
productC.numericAttributes["rating"] = 4.2;
productC.categoricalAttributes["category"] = "accessories";
actions["product-c"] = productC;

// Get bandit recommendation
eppoclient::BanditResult result = client.getBanditAction(
    "product-recommendation-flag",  // flag key
    "user-xyz-789",                  // subject key
    subjectAttributes,               // subject context
    actions,                         // available actions
    "control"                        // default variation
);

// Use the recommended action
if (result.action.has_value()) {
    std::string recommendedProduct = result.action.value();
    std::cout << "Recommending: " << recommendedProduct << std::endl;

    // Show the recommended product to the user
    if (recommendedProduct == "product-a") {
        // Display Product A
    } else if (recommendedProduct == "product-b") {
        // Display Product B
    } else if (recommendedProduct == "product-c") {
        // Display Product C
    }
} else {
    // No action selected, use default experience
    std::cout << "Using default recommendation" << std::endl;
}
```

### Complete Bandit Example

Here's a complete example from `examples/bandits.cpp` showing bandit-powered car recommendations:

```cpp
#include <iostream>
#include <memory>
#include "client.hpp"

int main() {
    // Load configuration
    std::string flagConfigJson = "...";  // Your flag config JSON
    std::string banditModelsJson = "...";  // Your bandit models JSON
    auto result = eppoclient::parseConfiguration(flagConfigJson, banditModelsJson);
    if (!result.hasValue()) {
        std::cerr << "Configuration parsing failed:" << std::endl;
        for (const auto& error : result.errors) {
            std::cerr << "  - " << error << std::endl;
        }
        return 1;
    }

    auto configStore = std::make_shared<eppoclient::ConfigurationStore>();
    configStore->setConfiguration(std::move(*result.value));

    // Create loggers
    auto assignmentLogger = std::make_shared<MyAssignmentLogger>();
    auto banditLogger = std::make_shared<MyBanditLogger>();
    auto applicationLogger = std::make_shared<MyApplicationLogger>();

    // Create client
    eppoclient::EppoClient client(
        configStore,
        assignmentLogger,
        banditLogger,
        applicationLogger
    );

    // Define subject attributes (user context)
    eppoclient::ContextAttributes subjectAttributes;
    // Add any relevant user attributes here

    // Define available car actions with their attributes
    std::map<std::string, eppoclient::ContextAttributes> actions;

    eppoclient::ContextAttributes toyota;
    toyota.numericAttributes["speed"] = 120.0;
    actions["toyota"] = toyota;

    eppoclient::ContextAttributes honda;
    honda.numericAttributes["speed"] = 115.0;
    actions["honda"] = honda;

    // Get bandit recommendation
    eppoclient::BanditResult result = client.getBanditAction(
        "car_bandit_flag",
        "user-abc123",
        subjectAttributes,
        actions,
        "car_bandit"
    );

    if (result.action.has_value()) {
        std::cout << "Recommended car: " << result.action.value() << std::endl;
    }

    return 0;
}
```

## Error Handling

The Eppo SDK is built with **`-fno-exceptions`** and does not use exceptions internally. When errors occur during flag evaluation (such as missing flags, invalid parameters, or type mismatches), the SDK:

1. **Logs the error** through the `ApplicationLogger` interface
2. **Returns the default value** you provided

See the **Getting Detailed Error Information** section below for more refined error handling.

### Error Handling Behavior

```cpp
auto configStore = std::make_shared<eppoclient::ConfigurationStore>();
// ... (after configStore->setConfiguration())

eppoclient::EppoClient client(
    configStore,
    assignmentLogger,
    nullptr,
    applicationLogger
);

eppoclient::Attributes attributes;

// If the flag doesn't exist, returns the default value (false)
// and logs an info message through applicationLogger
bool result = client.getBooleanAssignment(
    "non-existent-flag",
    "user-123",
    attributes,
    false  // This default value is returned
);
// result will be false

// If parameters are invalid (e.g., empty subject key),
// returns the default value and logs an error
bool result2 = client.getBooleanAssignment(
    "my-flag",
    "",  // Empty subject key
    attributes,
    true  // This default value is returned
);
// result2 will be true
```

### Monitoring Errors

To monitor errors, implement the `ApplicationLogger` interface:

```cpp
class MyApplicationLogger : public eppoclient::ApplicationLogger {
public:
    void error(const std::string& message) override {
        // Log to your monitoring system
        std::cerr << "[ERROR] " << message << std::endl;
        // Send to Sentry, CloudWatch, etc.
    }

    void warn(const std::string& message) override {
        std::cerr << "[WARN] " << message << std::endl;
    }

    void info(const std::string& message) override {
        std::cout << "[INFO] " << message << std::endl;
    }

    void debug(const std::string& message) override {
        std::cout << "[DEBUG] " << message << std::endl;
    }
};

auto logger = std::make_shared<MyApplicationLogger>();
auto configStore = std::make_shared<eppoclient::ConfigurationStore>();
// ... (after configStore->setConfiguration())
eppoclient::EppoClient client(configStore, nullptr, nullptr, logger);
```

### Getting Detailed Error Information

For more granular error handling, use the `*Details()` variants of assignment functions (such as `getBooleanAssignmentDetails()`, `getStringAssignmentDetails()`, etc.). These functions return evaluation details that include:

1. **Flag evaluation code**: Indicates why a particular assignment was made or what error occurred
2. **Flag evaluation details**: Contains specific error messages when errors are encountered

```cpp
eppoclient::Attributes attributes;

// Use the *Details function to get evaluation information
auto result = client.getBooleanAssignmentDetails(
    "my-flag",
    "user-123",
    attributes,
    false  // default value
);

// Check if evaluation details are available
if (result.evaluationDetails.has_value()) {
    eppoclient::EvaluationDetails details = (*result.evaluationDetails);

    // Check the flag evaluation code
    if (details.flagEvaluationCode.has_value()) {
        std::string code = eppoclient::flagEvaluationCodeToString(*details.flagEvaluationCode);
        std::cout << "Evaluation code: " << code << std::endl;
    }

    // Check for error messages
    if (details.flagEvaluationDetails.has_value()) {
        std::string message = *details.flagEvaluationDetails;
        if (!message.empty()) {
            std::cerr << "Error details: " << message << std::endl;
        }
    }
}

// Use the assigned value
bool isEnabled = result.variation;
```

This approach is especially useful when you need to handle specific error conditions differently or want to report detailed error information to monitoring systems.

### Common Error Scenarios

The SDK logs different message types for different scenarios:

- **Error**: Invalid parameters (empty flag key, empty subject key)
- **Warn**: Type mismatches (requesting wrong type for a flag)
- **Info**: Missing flag configurations, subject not in allocation

### Constructor Preconditions

Some constructors validate preconditions using `assert()` statements:

```cpp
// LruAssignmentLogger: inner logger must not be null, cache size > 0
auto logger = std::make_shared<MyAssignmentLogger>();
auto lruLogger = eppoclient::NewLruAssignmentLogger(logger, 1000);  // OK

// This will trigger an assertion failure in debug builds:
// auto badLogger = eppoclient::NewLruAssignmentLogger(nullptr, 1000);  // Assertion fails!
```

**Preconditions that trigger assertions in debug builds:**
- `LruAssignmentLogger`: `logger != nullptr`, `cacheSize > 0`
- `LruBanditLogger`: `logger != nullptr`, `cacheSize > 0`
- `TwoQueueCache`: `size > 0`

Always ensure these preconditions are met to avoid assertion failures.

### Complete Example with Error Monitoring

```cpp
#include <iostream>
#include <memory>
#include "client.hpp"

int main() {
    // Initialize client with application logger
    std::string configJson = "...";  // Your JSON config string
    auto result = eppoclient::parseConfiguration(configJson);
    if (!result.hasValue()) {
        std::cerr << "Configuration parsing failed:" << std::endl;
        for (const auto& error : result.errors) {
            std::cerr << "  - " << error << std::endl;
        }
        return 1;
    }

    auto configStore = std::make_shared<eppoclient::ConfigurationStore>();
    configStore->setConfiguration(std::move(*result.value));

    auto applicationLogger = std::make_shared<MyApplicationLogger>();
    eppoclient::EppoClient client(
        configStore,
        nullptr,
        nullptr,
        applicationLogger
    );

    eppoclient::Attributes attributes;
    attributes["company_id"] = std::string("42");

    // SDK handles errors gracefully - no exceptions thrown
    bool isEnabled = client.getBooleanAssignment(
        "new-checkout-flow",
        "user-123",
        attributes,
        false  // Default value
    );

    // Errors are logged through applicationLogger
    // Application continues normally
    if (isEnabled) {
        std::cout << "Using new checkout flow" << std::endl;
    } else {
        std::cout << "Using old checkout flow" << std::endl;
    }

    return 0;
}
```

### Compatibility with `-fno-exceptions` Projects

The SDK is fully compatible with projects built with `-fno-exceptions`:

```cmake
# Your project's CMakeLists.txt
add_executable(your_app main.cpp)
target_compile_options(your_app PRIVATE -fno-exceptions)
target_link_libraries(your_app PRIVATE eppoclient)  # Works!
```

The SDK library is built with `-fno-exceptions` internally, so it integrates seamlessly with exception-free codebases.

## Evaluation Details

When debugging flag assignments or understanding why a particular variant was selected, you can use evaluation details. The SDK provides detailed methods that return both the assigned value and metadata about the evaluation:

```cpp
// Get assignment with evaluation details
auto result = client.getStringAssignmentDetails(
    "button-color",
    "user-123",
    attributes,
    "blue"
);

// Access the assigned value
std::string color = result.variation;  // e.g., "green"
// Access evaluation metadata
if (result.evaluationDetails.has_value()) {
    eppoclient::EvaluationDetails details = (*result.evaluationDetails);
    if (details.flagEvaluationCode.has_value()) {
        std::cout << "Flag Evaluation Code: " << eppoclient::flagEvaluationCodeToString(*details.flagEvaluationCode) << std::endl;
    }
}
```

All assignment methods have corresponding `*Details()` variants:
- `getBooleanAssignmentDetails()`
- `getStringAssignmentDetails()`
- `getNumericAssignmentDetails()`
- `getIntegerAssignmentDetails()`
- `getJsonAssignmentDetails()`
- `getSerializedJsonAssignmentDetails()`
- `getBanditActionDetails()`

For more information on debugging flag assignments and using evaluation details, see the [Eppo SDK debugging documentation](https://docs.geteppo.com/sdks/sdk-features/debugging-flag-assignment#allocation-evaluation-scenarios). You can find working examples in [examples/assignment_details.cpp](https://github.com/Eppo-exp/cpp-sdk/blob/main/examples/assignment_details.cpp).

## EvaluationClient vs EppoClient

The SDK provides two client classes for different use cases:

### EppoClient (Recommended for Most Users)

`EppoClient` is the high-level client that manages configuration storage and provides optional logging:

```cpp
auto configStore = std::make_shared<eppoclient::ConfigurationStore>();
configStore->setConfiguration(config);

// Loggers are optional (can be nullptr)
eppoclient::EppoClient client(
    configStore,
    assignmentLogger,  // optional
    banditLogger,      // optional
    applicationLogger  // optional
);

bool result = client.getBooleanAssignment("flag-key", "user-123", attrs, false);
```

**Benefits:**
- Works with `ConfigurationStore` for easy configuration updates
- Optional loggers (can pass `nullptr`)
- Simpler API for most use cases

### EvaluationClient (Advanced Use Cases)

`EvaluationClient` is the low-level evaluation engine designed to **separate evaluation logic from state management**. Use this approach for more manual control over synchronization. 

```cpp
const Configuration& config = ...;  // Must outlive EvaluationClient
MyAssignmentLogger assignmentLogger;
MyBanditLogger banditLogger;
MyApplicationLogger applicationLogger;

eppoclient::EvaluationClient evaluationClient(
    config,
    assignmentLogger,  // required reference
    banditLogger,      // required reference
    applicationLogger  // required reference
);

bool result = evaluationClient.getBooleanAssignment("flag-key", "user-123", attrs, false);
```

**Design Philosophy:**

`EvaluationClient` was introduced to provide maximum flexibility and performance:

- **Zero synchronization overhead**: Takes configuration and loggers by reference with no shared pointers or mutex locking
- **Cheap construction**: Extremely lightweight to create and destroy instances
- **Flexible synchronization strategies**: Instead of forcing a one-size-fits-all locking approach (like protecting the entire client with a mutex), you can implement your own synchronization strategy around `ConfigurationStore`
- **Parallel evaluation**: Enables efficient concurrent evaluations—you can guard only the cheap `shared_ptr` copying operation when retrieving configuration, then evaluate in parallel
- **Custom configuration management**: Allows building your own configuration management system without being constrained by `ConfigurationStore`'s internal implementation

**When to use EvaluationClient:**
- You need maximum performance with custom synchronization strategies
- You want to evaluate flags in parallel across multiple threads with minimal locking
- You want direct control over the `Configuration` object lifetime

**Important notes:**
- All parameters (configuration and loggers) are passed by reference and must outlive the `EvaluationClient` instance
- All loggers are required (not optional)
- You're responsible for managing the `Configuration` lifetime and any necessary synchronization

**For most applications, use `EppoClient`**. Only use `EvaluationClient` if you need the advanced control and performance characteristics it provides.

For a complete working example of using `EvaluationClient` with manual synchronization, see [examples/manual_sync.cpp](https://github.com/Eppo-exp/cpp-sdk/blob/main/examples/manual_sync.cpp).

```cpp
auto configStore = std::make_shared<eppoclient::ConfigurationStore>();
configStore->setConfiguration(std::move(*result.value));

// Create thread-safe loggers
auto assignmentLogger = std::make_shared<ThreadSafeAssignmentLogger>();

eppoclient::EppoClient client(configStore, assignmentLogger);

// ✅ Safe to call from multiple threads without any additional synchronization
// Thread 1:
bool feature1 = client.getBooleanAssignment("flag1", "user-123", attrs, false);

// Thread 2:
bool feature2 = client.getBooleanAssignment("flag2", "user-456", attrs, false);

// Thread 3:
std::string variant = client.getStringAssignment("flag3", "user-789", attrs, "default");
```

### Updating Configuration

Configuration updates are also thread-safe and can happen concurrently with flag evaluations:

```cpp
// Thread 1: Evaluating flags
bool result = client.getBooleanAssignment("flag", "user", attrs, false);

// Thread 2: Updating configuration (safe!)
eppoclient::Configuration newConfig = ...;
configStore->setConfiguration(newConfig);

// Subsequent evaluations on Thread 1 will use the new configuration
```

### Advanced: EvaluationClient for Maximum Performance

For advanced use cases requiring maximum performance, you can use `EvaluationClient` directly with custom synchronization strategies. This approach avoids creating temporary objects on each evaluation:

```cpp
// Get configuration once (thread-safe)
auto config = configStore->getConfiguration();

// Create long-lived EvaluationClient (cheap, no locking)
eppoclient::EvaluationClient evaluationClient(*config, assignmentLogger,
                                             banditLogger, applicationLogger);

// Evaluate many flags without any locking overhead
bool result1 = evaluationClient.getBooleanAssignment("flag1", "user", attrs, false);
bool result2 = evaluationClient.getBooleanAssignment("flag2", "user", attrs, false);
// ... thousands more evaluations ...
```

For a complete example of this advanced pattern, see [examples/manual_sync.cpp](https://github.com/Eppo-exp/cpp-sdk/blob/main/examples/manual_sync.cpp).

### Design Philosophy

The SDK's thread-safety design provides:
- **Zero synchronization overhead** - No mutexes during flag evaluation
- **Immutable configurations** - Safe concurrent access without locking
- **Atomic configuration updates** - Updates don't block ongoing evaluations
- **Simple API** - No need for wrapper classes or manual locking in most cases

### Important Notes

- `ConfigurationStore` must outlive any `EppoClient` instances that reference it
- Configuration objects retrieved via `getConfiguration()` remain valid even if the store is updated
- Logger interfaces must be thread-safe if used from multiple threads (use mutexes in logger implementations if needed)
- `EvaluationClient` instances are lightweight and cheap to create per-evaluation if needed

## Additional Resources

- Full working examples in the `examples/` directory
- See [examples/flag_assignments.cpp](https://github.com/Eppo-exp/cpp-sdk/blob/main/examples/flag_assignments.cpp) for feature flag examples
- See [examples/bandits.cpp](https://github.com/Eppo-exp/cpp-sdk/blob/main/examples/bandits.cpp) for contextual bandit examples
- See [examples/assignment_details.cpp](https://github.com/Eppo-exp/cpp-sdk/blob/main/examples/assignment_details.cpp) for evaluation details examples
- See [examples/manual_sync.cpp](https://github.com/Eppo-exp/cpp-sdk/blob/main/examples/manual_sync.cpp) for advanced usage with EvaluationClient and manual synchronization


# Eppo C++ SDK

[Eppo](https://geteppo.com) is a modular flagging and experimentation analysis tool.

## Features

The Eppo C++ SDK is designed for offline flag evaluation and supports:

- **Feature Flags**: Boolean, numeric, integer, string, and JSON flag evaluations
- **Contextual Bandits**: ML-powered dynamic variant selection
- **Assignment Logging**: Track which variants are assigned to users, with automatic deduplication
- **Contextual Bandit Logging**: Track bandit actions, with automatic deduplication
- **Application Logging**: Debug and monitor SDK behavior

## Installation

### Using CMake FetchContent (Recommended)

The easiest way to integrate the Eppo C++ SDK is using CMake's FetchContent:

```cmake
include(FetchContent)
FetchContent_Declare(
  eppoclient
  GIT_REPOSITORY https://github.com/Eppo-exp/cpp-sdk.git
  GIT_TAG v1.0.0  # Use the latest version
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

No external dependencies required - all necessary libraries are vendored.

## Quick Start

### 1. Initialize the SDK

The Eppo SDK requires configuration data containing your feature flags. This SDK is designed for offline use, so you'll load configuration directly rather than using SDK keys or polling.

```cpp
#include <nlohmann/json.hpp>
#include "client.hpp"

// Parse configuration from a JSON string
eppoclient::ConfigResponse parseConfiguration(const std::string& configJson) {
    nlohmann::json j = nlohmann::json::parse(configJson);
    return j;
}

// Your configuration as a JSON string
std::string configJson = R"({
    "flags": {
        "new-ui-rollout": {
            "enabled": true,
            "variations": {...}
        }
    }
})";

// Create and initialize the configuration store
eppoclient::ConfigurationStore configStore;
eppoclient::ConfigResponse config = parseConfiguration(configJson);
configStore.setConfiguration(eppoclient::Configuration(config));

// Create the client (configStore must outlive client)
eppoclient::EppoClient client(configStore);
```

### 2. Evaluate Feature Flags

Once initialized, you can evaluate feature flags for different types:

```cpp
// Boolean flag
bool showNewFeature = client.getBoolAssignment(
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
    eppoclient::ConfigurationStore configStore;
    std::string configJson = "...";  // Your JSON config string
    eppoclient::ConfigResponse config = parseConfiguration(configJson);
    configStore.setConfiguration(eppoclient::Configuration(config));

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
    bool showNewUI = client.getBoolAssignment(
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
#include <nlohmann/json.hpp>
#include "client.hpp"

// Parse bandit models from a JSON string
eppoclient::BanditResponse parseBanditModels(const std::string& modelsJson) {
    nlohmann::json j = nlohmann::json::parse(modelsJson);
    return j;
}

// Your configuration and bandit models as JSON strings
std::string flagConfigJson = "...";  // Your flag config JSON
std::string banditModelsJson = "...";  // Your bandit models JSON

// Initialize with both flags and bandit models
eppoclient::ConfigResponse flagConfig = parseConfiguration(flagConfigJson);
eppoclient::BanditResponse banditModels = parseBanditModels(banditModelsJson);

eppoclient::ConfigurationStore configStore;
configStore.setConfiguration(eppoclient::Configuration(flagConfig, banditModels));

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
    eppoclient::ConfigurationStore configStore;
    std::string flagConfigJson = "...";  // Your flag config JSON
    std::string banditModelsJson = "...";  // Your bandit models JSON
    eppoclient::ConfigResponse flagConfig = parseConfiguration(flagConfigJson);
    eppoclient::BanditResponse banditModels = parseBanditModels(banditModelsJson);
    configStore.setConfiguration(eppoclient::Configuration(flagConfig, banditModels));

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

## Error Handling and Graceful Failure Mode

By default, the Eppo SDK operates in **graceful failure mode**, which means errors are logged but don't throw exceptions. Instead, the SDK returns the default value you provided. This is useful for production environments where you want your application to continue running even if flag evaluation fails.

### Default Behavior (Graceful Mode)

```cpp
eppoclient::EppoClient client(
    configStore,
    assignmentLogger,
    nullptr,
    applicationLogger
);

// By default, errors are logged but don't throw exceptions
eppoclient::Attributes attributes;
bool result = client.getBoolAssignment(
    "non-existent-flag",
    "user-123",
    attributes,
    false  // Returns this default value if flag doesn't exist
);
// result will be false, and an error will be logged to applicationLogger
```

### Strict Mode (Throw Exceptions)

For development, testing, or cases where you want to be notified immediately of errors, you can disable graceful failure mode using `setIsGracefulFailureMode(false)`. When disabled, the SDK will throw exceptions on errors instead of returning default values.

```cpp
eppoclient::EppoClient client(
    configStore,
    assignmentLogger,
    nullptr,
    applicationLogger
);

// Disable graceful failure mode - exceptions will be thrown
client.setIsGracefulFailureMode(false);

try {
    eppoclient::Attributes attributes;

    // This will throw std::invalid_argument if subject key is empty
    bool result = client.getBoolAssignment(
        "my-flag",
        "",  // Empty subject key - error!
        attributes,
        false
    );
} catch (const std::invalid_argument& e) {
    std::cerr << "Flag evaluation error: " << e.what() << std::endl;
    // Handle the error appropriately
}

try {
    // This will throw an exception if the flag doesn't exist
    bool result = client.getBoolAssignment(
        "non-existent-flag",
        "user-123",
        attributes,
        false
    );
} catch (const std::exception& e) {
    std::cerr << "Flag not found: " << e.what() << std::endl;
}
```

### Common Exception Types

When graceful failure mode is disabled, the SDK may throw:

- `std::invalid_argument` - Invalid parameters (e.g., empty flag key or subject key)
- `std::runtime_error` - Configuration or evaluation errors
- `std::exception` - Other general errors

### When to Use Each Mode

**Graceful Mode (default, `setIsGracefulFailureMode(true)`)**:
- Production environments
- When you want maximum uptime
- When default values are acceptable fallbacks
- When errors are monitored through application logs

**Strict Mode (`setIsGracefulFailureMode(false)`)**:
- Development and testing
- When you want immediate feedback on configuration issues
- When silent failures could cause problems
- When you need to handle errors explicitly in your code

### Complete Example with Error Handling

```cpp
#include <iostream>
#include <memory>
#include <exception>
#include "client.hpp"

int main() {
    try {
        // Initialize client
        eppoclient::ConfigurationStore configStore;
        eppoclient::ConfigResponse config = parseConfiguration("config/flags-v1.json");
        configStore.setConfiguration(eppoclient::Configuration(config));

        auto applicationLogger = std::make_shared<MyApplicationLogger>();
        eppoclient::EppoClient client(
            configStore,
            nullptr,
            nullptr,
            applicationLogger
        );

        // Enable strict mode for development
        client.setIsGracefulFailureMode(false);

        eppoclient::Attributes attributes;
        attributes["company_id"] = std::string("42");

        try {
            // Evaluate flag - will throw if there are issues
            bool isEnabled = client.getBoolAssignment(
                "new-checkout-flow",
                "user-123",
                attributes,
                false
            );

            if (isEnabled) {
                std::cout << "Using new checkout flow" << std::endl;
            } else {
                std::cout << "Using old checkout flow" << std::endl;
            }
        } catch (const std::invalid_argument& e) {
            std::cerr << "Invalid flag parameters: " << e.what() << std::endl;
            // Use fallback logic
        } catch (const std::exception& e) {
            std::cerr << "Flag evaluation failed: " << e.what() << std::endl;
            // Use fallback logic
        }

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
```

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
- `getBoolAssignmentDetails()`
- `getStringAssignmentDetails()`
- `getNumericAssignmentDetails()`
- `getIntegerAssignmentDetails()`
- `getJSONAssignmentDetails()`
- `getSerializedJSONAssignmentDetails()`
- `getBanditActionDetails()`

For more information on debugging flag assignments and using evaluation details, see the [Eppo SDK debugging documentation](https://docs.geteppo.com/sdks/sdk-features/debugging-flag-assignment#allocation-evaluation-scenarios). You can find working examples in [examples/assignment_details.cpp](https://github.com/Eppo-exp/cpp-sdk/blob/main/examples/assignment_details.cpp).

## Thread Safety and Concurrency

The Eppo C++ SDK is **not thread-safe by design**. If you need to use the SDK from multiple threads, you are responsible for providing appropriate synchronization mechanisms in your application.

### Key Points

- `ConfigurationStore` is not thread-safe - concurrent reads and writes require external synchronization
- `EppoClient` is not thread-safe - concurrent flag evaluations require external synchronization
- The caller is responsible for implementing any required synchronization (mutexes, locks, etc.)

### Single-Threaded Usage (No Synchronization Required)

If your application evaluates flags from a single thread, no special synchronization is needed:

```cpp
eppoclient::ConfigurationStore configStore;
configStore.setConfiguration(eppoclient::Configuration(config));

eppoclient::EppoClient client(configStore, assignmentLogger);

// All flag evaluations happen on the same thread - safe
bool feature1 = client.getBoolAssignment("flag1", "user-123", attrs, false);
bool feature2 = client.getBoolAssignment("flag2", "user-123", attrs, false);
```

### Multi-Threaded Usage (Synchronization Required)

If you need to evaluate flags from multiple threads or update configuration while reading, you must provide synchronization:

```cpp
#include <mutex>
#include <memory>

// Wrap the configuration store and client with a mutex
class ThreadSafeEppoClient {
private:
    eppoclient::ConfigurationStore configStore_;
    std::unique_ptr<eppoclient::EppoClient> client_;
    mutable std::mutex mutex_;

public:
    ThreadSafeEppoClient(
        std::shared_ptr<eppoclient::AssignmentLogger> assignmentLogger = nullptr,
        std::shared_ptr<eppoclient::BanditLogger> banditLogger = nullptr,
        std::shared_ptr<eppoclient::ApplicationLogger> applicationLogger = nullptr
    ) : client_(std::make_unique<eppoclient::EppoClient>(
            configStore_, assignmentLogger, banditLogger, applicationLogger)) {}

    // Thread-safe configuration update
    void updateConfiguration(const eppoclient::Configuration& config) {
        std::lock_guard<std::mutex> lock(mutex_);
        configStore_.setConfiguration(config);
    }

    // Thread-safe flag evaluation
    bool getBoolAssignment(
        const std::string& flagKey,
        const std::string& subjectKey,
        const eppoclient::Attributes& attributes,
        bool defaultValue
    ) {
        std::lock_guard<std::mutex> lock(mutex_);
        return client_->getBoolAssignment(flagKey, subjectKey, attributes, defaultValue);
    }

    // Add other methods as needed...
};

// Usage:
ThreadSafeEppoClient client(assignmentLogger);

// Can now safely call from multiple threads
bool result1 = client.getBoolAssignment("flag1", "user-123", attrs, false);
bool result2 = client.getBoolAssignment("flag2", "user-456", attrs, false);
```

### Design Philosophy

This design choice allows:
- **Maximum flexibility**: Applications can choose their preferred synchronization strategy (mutexes, read-write locks, lock-free structures, etc.)
- **Zero overhead for single-threaded applications**: No unnecessary locking when concurrency isn't needed
- **Better integration**: The SDK adapts to your application's existing concurrency model rather than imposing its own

### Important Notes

- `ConfigurationStore` must outlive any `EppoClient` instances that reference it
- If you share a `ConfigurationStore` across threads, protect both the store and all clients that reference it
- Logger interfaces (`AssignmentLogger`, `BanditLogger`, `ApplicationLogger`) should be thread-safe if used concurrently

## Additional Resources

- Full working examples in the `examples/` directory
- See [examples/flag_assignments.cpp](https://github.com/Eppo-exp/cpp-sdk/blob/main/examples/flag_assignments.cpp) for feature flag examples
- See [examples/bandits.cpp](https://github.com/Eppo-exp/cpp-sdk/blob/main/examples/bandits.cpp) for contextual bandit examples
- See [examples/assignment_details.cpp](https://github.com/Eppo-exp/cpp-sdk/blob/main/examples/assignment_details.cpp) for evaluation details examples


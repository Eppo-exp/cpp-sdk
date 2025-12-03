#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include "../src/client.hpp"
#include "../src/evaluation_client.hpp"

/**
 * Example: Using EvaluationClient with Manual Synchronization
 *
 * This example demonstrates how to use EvaluationClient for advanced use cases where
 * you need full control over configuration management and synchronization strategy.
 *
 * Key concepts demonstrated:
 * 1. Using EvaluationClient instead of EppoClient for maximum control
 * 2. Implementing custom synchronization around ConfigurationStore
 * 3. Separating configuration retrieval from evaluation for better parallelism
 * 4. Managing Configuration lifetime explicitly
 *
 * When to use EvaluationClient:
 * - You need maximum performance with custom synchronization strategies
 * - You're building a custom configuration management system
 * - You want to evaluate flags in parallel with minimal locking
 * - You need direct control over Configuration object lifetime
 *
 * For most applications, use EppoClient instead - it provides a simpler API
 * with built-in configuration management and optional loggers.
 */

// Simple console-based assignment logger
class ConsoleAssignmentLogger : public eppoclient::AssignmentLogger {
public:
    void logAssignment(const eppoclient::AssignmentEvent& event) override {
        std::cout << "\n=== Assignment Log ===" << std::endl;
        std::cout << "Feature Flag: " << event.featureFlag << std::endl;
        std::cout << "Variation: " << event.variation << std::endl;
        std::cout << "Subject: " << event.subject << std::endl;
        std::cout << "Timestamp: " << event.timestamp << std::endl;
        std::cout << "=====================\n" << std::endl;
    }
};

// Simple console-based bandit logger
class ConsoleBanditLogger : public eppoclient::BanditLogger {
public:
    void logBanditAction(const eppoclient::BanditEvent& event) override {
        std::cout << "\n=== Bandit Action Log ===" << std::endl;
        std::cout << "Flag Key: " << event.flagKey << std::endl;
        std::cout << "Action: " << event.action << std::endl;
        std::cout << "Action Probability: " << event.actionProbability << std::endl;
        std::cout << "Subject: " << event.subject << std::endl;
        std::cout << "========================\n" << std::endl;
    }
};

// Simple console-based application logger
class ConsoleApplicationLogger : public eppoclient::ApplicationLogger {
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

// Helper function to load flags configuration from JSON file
bool loadFlagsConfiguration(const std::string& filepath, eppoclient::Configuration& config) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open flags configuration file: " << filepath << std::endl;
        return false;
    }

    std::string configJson((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    auto result = eppoclient::parseConfiguration(configJson);

    if (!result.hasValue()) {
        std::cerr << "Failed to parse configuration:" << std::endl;
        for (const auto& error : result.errors) {
            std::cerr << "  - " << error << std::endl;
        }
        return false;
    }

    config = std::move(*result.value);
    return true;
}

/**
 * Custom Configuration Manager with Manual Synchronization
 *
 * This class demonstrates how to implement your own synchronization strategy
 * around ConfigurationStore. The key insight is that you only need to protect
 * the getConfiguration() call with a mutex - the actual flag evaluation can
 * happen in parallel without any locking because Configuration is immutable.
 *
 * This approach provides better performance than protecting the entire
 * EvaluationClient with a mutex, especially when evaluating many flags.
 */
class ManualSyncConfigManager {
private:
    std::shared_ptr<eppoclient::ConfigurationStore> configStore_;
    mutable std::mutex mutex_;

public:
    ManualSyncConfigManager() : configStore_(std::make_shared<eppoclient::ConfigurationStore>()) {}

    /**
     * Thread-safe configuration update
     *
     * Note: ConfigurationStore::setConfiguration() is internally thread-safe
     * (uses atomic operations), so we technically don't need the mutex here.
     * However, we include it for demonstration purposes and to show a complete
     * synchronization pattern.
     */
    void updateConfiguration(const eppoclient::Configuration& config) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << "[ConfigManager] Updating configuration..." << std::endl;
        configStore_->setConfiguration(config);
        std::cout << "[ConfigManager] Configuration updated successfully" << std::endl;
    }

    /**
     * Thread-safe configuration retrieval
     *
     * This is the critical operation that needs protection. We use the mutex
     * only to safely copy the shared_ptr, which is very fast. After we have
     * the shared_ptr, we can release the lock and evaluate flags in parallel.
     */
    std::shared_ptr<const eppoclient::Configuration> getConfiguration() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return configStore_->getConfiguration();
    }
};

int main() {
    std::cout << "=== EvaluationClient with Manual Synchronization Example ===\n" << std::endl;

    // Step 1: Load initial configuration
    std::cout << "Step 1: Loading initial configuration from file..." << std::endl;
    eppoclient::Configuration initialConfig;
    if (!loadFlagsConfiguration("config/flags-v1.json", initialConfig)) {
        return 1;
    }
    std::cout << "Initial configuration loaded successfully\n" << std::endl;

    // Step 2: Create configuration manager with manual synchronization
    std::cout << "Step 2: Setting up configuration manager..." << std::endl;
    ManualSyncConfigManager configManager;
    configManager.updateConfiguration(initialConfig);
    std::cout << "" << std::endl;

    // Step 3: Create loggers (required for EvaluationClient)
    std::cout << "Step 3: Creating loggers..." << std::endl;
    ConsoleAssignmentLogger assignmentLogger;
    ConsoleBanditLogger banditLogger;
    ConsoleApplicationLogger applicationLogger;
    std::cout << "Loggers created\n" << std::endl;

    // Step 4: Demonstrate manual synchronization pattern
    std::cout << "Step 4: Evaluating flags with EvaluationClient\n" << std::endl;
    std::cout << "--- Approach: Manual synchronization for optimal performance ---" << std::endl;
    std::cout << "The pattern is:" << std::endl;
    std::cout << "1. Lock mutex briefly to get Configuration shared_ptr (fast!)" << std::endl;
    std::cout << "2. Release mutex immediately" << std::endl;
    std::cout << "3. Create EvaluationClient with Configuration reference" << std::endl;
    std::cout << "4. Evaluate flags without any locking (Configuration is immutable)\n"
              << std::endl;

    // Get configuration (this is the only part that needs locking)
    std::cout << "[Main] Retrieving configuration from manager..." << std::endl;
    auto config = configManager.getConfiguration();
    std::cout << "[Main] Configuration retrieved (mutex released)\n" << std::endl;

    // Create EvaluationClient (cheap operation, no locking needed)
    // Note: All parameters are passed by reference and must outlive the client
    std::cout << "[Main] Creating EvaluationClient..." << std::endl;
    eppoclient::EvaluationClient evaluationClient(*config, assignmentLogger, banditLogger,
                                                  applicationLogger);
    std::cout << "[Main] EvaluationClient created\n" << std::endl;

    // Step 5: Evaluate flags (no locking needed - Configuration is immutable)
    std::cout << "Step 5: Evaluating multiple flags (no mutex contention!)\n" << std::endl;

    // Define subject attributes
    eppoclient::Attributes attributes1;
    attributes1["should_disable_feature"] = false;

    std::cout << "=== Test 1: Boolean flag evaluation ===" << std::endl;
    bool boolResult = evaluationClient.getBooleanAssignment("boolean-false-assignment",
                                                            "user-alice", attributes1, false);
    std::cout << "Result: " << (boolResult ? "true" : "false") << std::endl;

    std::cout << "\n=== Test 2: String assignment with different subject ===" << std::endl;
    eppoclient::Attributes attributes2;
    attributes2["should_disable_feature"] = true;
    bool boolResult2 = evaluationClient.getBooleanAssignment("boolean-false-assignment", "user-bob",
                                                             attributes2, false);
    std::cout << "Result: " << (boolResult2 ? "true" : "false") << std::endl;

    std::cout << "\n=== Test 3: JSON assignment ===" << std::endl;
    eppoclient::Attributes attributes3;
    attributes3["Force Empty"] = "false";
    std::string jsonResult = evaluationClient.getSerializedJSONAssignment(
        "json-config-flag", "user-charlie", attributes3,
        "{\"integer\": 0, \"string\": \"default\", \"float\": 0.0}");

    nlohmann::json jsonObj = nlohmann::json::parse(jsonResult);
    std::cout << "JSON result: " << jsonResult << std::endl;
    std::cout << "Parsed values:" << std::endl;
    std::cout << "  integer: " << jsonObj["integer"] << std::endl;
    std::cout << "  string: " << jsonObj["string"] << std::endl;
    std::cout << "  float: " << jsonObj["float"] << std::endl;

    // Step 6: Demonstrate configuration updates
    std::cout << "\n\nStep 6: Demonstrating configuration updates\n" << std::endl;
    std::cout << "In a real application, you might periodically fetch new configuration"
              << std::endl;
    std::cout << "from a server or file and update the ConfigurationStore." << std::endl;
    std::cout << "\nSimulating configuration update..." << std::endl;

    // Load configuration again (in real app, this might be new config from server)
    eppoclient::Configuration updatedConfig;
    if (loadFlagsConfiguration("config/flags-v1.json", updatedConfig)) {
        configManager.updateConfiguration(updatedConfig);
        std::cout << "Configuration updated! New evaluations will use updated config.\n"
                  << std::endl;

        // Get new configuration and create new evaluation client
        auto newConfig = configManager.getConfiguration();
        eppoclient::EvaluationClient newEvaluationClient(*newConfig, assignmentLogger, banditLogger,
                                                         applicationLogger);

        std::cout << "=== Test 4: Evaluation with updated configuration ===" << std::endl;
        bool updatedResult = newEvaluationClient.getBooleanAssignment(
            "boolean-false-assignment", "user-dave", attributes1, false);
        std::cout << "Result with updated config: " << (updatedResult ? "true" : "false")
                  << std::endl;
    }

    // Step 7: Demonstrate evaluation details
    std::cout << "\n\nStep 7: Using evaluation details for debugging\n" << std::endl;
    std::cout << "EvaluationClient also supports *Details() methods for debugging:" << std::endl;

    auto detailsResult = evaluationClient.getBooleanAssignmentDetails(
        "boolean-false-assignment", "user-eve", attributes1, false);

    std::cout << "\n=== Test 5: Boolean assignment with details ===" << std::endl;
    std::cout << "Variation: " << (detailsResult.variation ? "true" : "false") << std::endl;

    if (detailsResult.evaluationDetails.has_value()) {
        const auto& details = *detailsResult.evaluationDetails;
        std::cout << "Evaluation details available:" << std::endl;
        std::cout << "  Flag Key: " << details.flagKey << std::endl;
        std::cout << "  Subject Key: " << details.subjectKey << std::endl;

        if (details.flagEvaluationCode.has_value()) {
            std::string code = eppoclient::flagEvaluationCodeToString(*details.flagEvaluationCode);
            std::cout << "  Evaluation Code: " << code << std::endl;
        }

        if (!details.flagEvaluationDescription.empty()) {
            std::cout << "  Description: " << details.flagEvaluationDescription << std::endl;
        }
    }

    // Summary
    std::cout << "\n\n=== Summary ===" << std::endl;
    std::cout << "This example demonstrated:" << std::endl;
    std::cout << "1. Using EvaluationClient for direct flag evaluation" << std::endl;
    std::cout << "2. Implementing custom synchronization around ConfigurationStore" << std::endl;
    std::cout << "3. Separating config retrieval (fast, locked) from evaluation (unlocked)"
              << std::endl;
    std::cout << "4. Updating configuration at runtime" << std::endl;
    std::cout << "5. Using evaluation details for debugging" << std::endl;
    std::cout << "\nKey benefits of this approach:" << std::endl;
    std::cout << "- Maximum performance: minimal locking, parallel evaluation" << std::endl;
    std::cout << "- Flexibility: implement your own synchronization strategy" << std::endl;
    std::cout << "- Direct control: manage Configuration lifetime explicitly" << std::endl;
    std::cout << "\nFor a more automated approach, use EppoClient instead!" << std::endl;

    return 0;
}

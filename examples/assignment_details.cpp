#include <fstream>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include "../src/client.hpp"

// Helper function to print evaluation details
void printEvaluationDetails(const eppoclient::EvaluationDetails& details) {
    std::cout << "\n=== Evaluation Details ===" << std::endl;
    std::cout << "Flag Key: " << details.flagKey << std::endl;
    std::cout << "Subject Key: " << details.subjectKey << std::endl;
    std::cout << "Timestamp: " << details.timestamp << std::endl;

    if (details.flagEvaluationCode.has_value()) {
        std::cout << "Flag Evaluation Code: "
                  << eppoclient::flagEvaluationCodeToString(*details.flagEvaluationCode)
                  << std::endl;
    }
    std::cout << "Flag Evaluation Description: " << details.flagEvaluationDescription << std::endl;

    if (details.variationKey.has_value()) {
        std::cout << "Variation Key: " << *details.variationKey << std::endl;
    }

    if (details.configFetchedAt.has_value()) {
        std::cout << "Config Fetched At: " << *details.configFetchedAt << std::endl;
    }

    if (details.environmentName.has_value()) {
        std::cout << "Environment Name: " << *details.environmentName << std::endl;
    }

    std::cout << "========================\n" << std::endl;
}

// Simple console-based assignment logger
class ConsoleAssignmentLogger : public eppoclient::AssignmentLogger {
public:
    void logAssignment(const eppoclient::AssignmentEvent& event) override {
        std::cout << "\n=== Assignment Log ===" << std::endl;
        std::cout << "Experiment: " << event.experiment << std::endl;
        std::cout << "Feature Flag: " << event.featureFlag << std::endl;
        std::cout << "Allocation: " << event.allocation << std::endl;
        std::cout << "Variation: " << event.variation << std::endl;
        std::cout << "Subject: " << event.subject << std::endl;
        std::cout << "Timestamp: " << event.timestamp << std::endl;
        std::cout << "=====================\n" << std::endl;
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
bool loadFlagsConfiguration(const std::string& filepath, eppoclient::ConfigResponse& response) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open flags configuration file: " << filepath << std::endl;
        return false;
    }

    nlohmann::json j;
    file >> j;

    response = j;
    return true;
}

int main() {
    // Load the flags configuration
    std::cout << "Loading flags configuration..." << std::endl;
    eppoclient::ConfigResponse ufc;
    if (!loadFlagsConfiguration("config/flags-v1.json", ufc)) {
        return 1;
    }

    // Create configuration store and set the configuration
    eppoclient::ConfigurationStore configStore;
    configStore.setConfiguration(eppoclient::Configuration(ufc));

    // Create assignment logger and application logger
    auto assignmentLogger = std::make_shared<ConsoleAssignmentLogger>();
    auto applicationLogger = std::make_shared<ConsoleApplicationLogger>();

    // Create EppoClient
    eppoclient::EppoClient client(configStore, assignmentLogger, nullptr, applicationLogger);

    // Example 1: Boolean assignment with details
    std::cout << "\n=== Example 1: Boolean Assignment with Details ===" << std::endl;
    eppoclient::Attributes attributes1;
    attributes1["should_disable_feature"] = false;
    auto result1 = client.getBooleanAssignmentDetails("boolean-false-assignment", "user-123",
                                                      attributes1, false);
    std::cout << "Boolean Result: " << (result1.variation ? "true" : "false") << std::endl;
    if (result1.evaluationDetails.has_value()) {
        printEvaluationDetails(*result1.evaluationDetails);
    }

    // Example 2: String assignment with details
    std::cout << "\n=== Example 2: String Assignment with Details ===" << std::endl;
    eppoclient::Attributes attributes2;
    auto result2 =
        client.getStringAssignmentDetails("kill-switch", "user-456", attributes2, "default-value");
    std::cout << "String Result: " << result2.variation << std::endl;
    if (result2.evaluationDetails.has_value()) {
        printEvaluationDetails(*result2.evaluationDetails);
    }

    // Example 3: Integer assignment with details
    std::cout << "\n=== Example 3: Integer Assignment with Details ===" << std::endl;
    eppoclient::Attributes attributes3;
    attributes3["age"] = 25.0;
    auto result3 = client.getIntegerAssignmentDetails("integer-flag", "user-789", attributes3, 42);
    std::cout << "Integer Result: " << result3.variation << std::endl;
    if (result3.evaluationDetails.has_value()) {
        printEvaluationDetails(*result3.evaluationDetails);
    }

    // Example 4: Numeric assignment with details
    std::cout << "\n=== Example 4: Numeric Assignment with Details ===" << std::endl;
    eppoclient::Attributes attributes4;
    auto result4 =
        client.getNumericAssignmentDetails("numeric_flag", "user-999", attributes4, 3.14);
    std::cout << "Numeric Result: " << result4.variation << std::endl;
    if (result4.evaluationDetails.has_value()) {
        printEvaluationDetails(*result4.evaluationDetails);
    }

    // Example 5: Non-existent flag (demonstrates error handling)
    std::cout << "\n=== Example 5: Non-existent Flag ===" << std::endl;
    auto result5 = client.getStringAssignmentDetails("non-existent-flag", "user-404",
                                                     eppoclient::Attributes(), "fallback-value");
    std::cout << "Result: " << result5.variation << std::endl;
    if (result5.evaluationDetails.has_value()) {
        printEvaluationDetails(*result5.evaluationDetails);
    }

    std::cout << "\n=== All Examples Completed ===" << std::endl;
    return 0;
}

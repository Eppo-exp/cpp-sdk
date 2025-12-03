#include <fstream>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include "../src/client.hpp"

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

        if (!event.subjectAttributes.empty()) {
            std::cout << "Subject Attributes:" << std::endl;
            for (const auto& [key, value] : event.subjectAttributes) {
                std::cout << "  " << key << ": ";
                std::visit(
                    [](const auto& v) {
                        using T = std::decay_t<decltype(v)>;
                        if constexpr (std::is_same_v<T, std::monostate>) {
                            std::cout << "(none)";
                        } else if constexpr (std::is_same_v<T, bool>) {
                            std::cout << (v ? "true" : "false");
                        } else {
                            std::cout << v;
                        }
                    },
                    value);
                std::cout << std::endl;
            }
        }

        if (!event.metaData.empty()) {
            std::cout << "Metadata:" << std::endl;
            for (const auto& [key, value] : event.metaData) {
                std::cout << "  " << key << ": " << value << std::endl;
            }
        }
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
bool loadFlagsConfiguration(const std::string& filepath, eppoclient::Configuration& config) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open flags configuration file: " << filepath << std::endl;
        return false;
    }

    // Read entire file content into a string
    std::string configJson((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    // Parse configuration using parseConfiguration
    auto result = eppoclient::parseConfiguration(configJson);

    if (!result.hasValue()) {
        std::cerr << "Failed to parse configuration:" << std::endl;
        for (const auto& error : result.errors) {
            std::cerr << "  - " << error << std::endl;
        }
        return false;
    }

    // Report any warnings
    if (result.hasErrors()) {
        std::cerr << "Configuration parsing had errors:" << std::endl;
        for (const auto& error : result.errors) {
            std::cerr << "  - " << error << std::endl;
        }
    }

    config = std::move(*result.value);
    return true;
}

int main() {
    // Load the flags configuration
    std::cout << "Loading flags configuration..." << std::endl;
    eppoclient::Configuration config;
    if (!loadFlagsConfiguration("config/flags-v1.json", config)) {
        return 1;
    }

    // Create configuration store and set the configuration
    auto configStore = std::make_shared<eppoclient::ConfigurationStore>();
    configStore->setConfiguration(config);

    // Create assignment logger and application logger
    auto assignmentLogger = std::make_shared<ConsoleAssignmentLogger>();
    auto applicationLogger = std::make_shared<ConsoleApplicationLogger>();

    // Create EppoClient with all parameters (nullptr for banditLogger since this example doesn't
    // use bandits)
    eppoclient::EppoClient client(configStore, assignmentLogger, nullptr, applicationLogger);

    // Test 1: No matching attributes (should use default value, no assignment log)
    std::cout << "\n=== Test 1: No matching attributes ===" << std::endl;
    eppoclient::Attributes attributes1;
    attributes1["company_id"] = "42";
    bool result1 =
        client.getBooleanAssignment("boolean-false-assignment", "my-subject", attributes1, false);
    if (result1) {
        std::cout << "Hello Universe!" << std::endl;
    } else {
        std::cout << "Hello World!" << std::endl;
    }

    // Test 2: should_disable_feature = false (should match allocation, log assignment)
    std::cout << "\n=== Test 2: should_disable_feature = false ===" << std::endl;
    eppoclient::Attributes attributes2;
    attributes2["should_disable_feature"] = false;
    bool result2 =
        client.getBooleanAssignment("boolean-false-assignment", "my-subject", attributes2, false);
    if (result2) {
        std::cout << "Hello Universe!" << std::endl;
    } else {
        std::cout << "Hello World!" << std::endl;
    }

    // Test 3: should_disable_feature = true (should match allocation, log assignment)
    std::cout << "\n=== Test 3: should_disable_feature = true ===" << std::endl;
    eppoclient::Attributes attributes3;
    attributes3["should_disable_feature"] = true;
    bool result3 =
        client.getBooleanAssignment("boolean-false-assignment", "my-subject", attributes3, false);
    if (result3) {
        std::cout << "Hello Universe!" << std::endl;
    } else {
        std::cout << "Hello World!" << std::endl;
    }

    // Test 4: getSerializedJSONAssignment (should match allocation, log assignment, return JSON
    // string)
    std::cout << "\n=== Test 4: getSerializedJSONAssignment ===" << std::endl;
    eppoclient::Attributes attributes4;
    attributes4["Force Empty"] = "false";
    std::string jsonResult = client.getSerializedJSONAssignment(
        "json-config-flag", "user-123", attributes4,
        "{\"integer\": 0, \"string\": \"default\", \"float\": 0.0}");

    // Parse the returned JSON string
    nlohmann::json jsonObj = nlohmann::json::parse(jsonResult);
    std::cout << "Received JSON assignment: " << jsonResult << std::endl;
    std::cout << "Parsed values:" << std::endl;
    std::cout << "  integer: " << jsonObj["integer"] << std::endl;
    std::cout << "  string: " << jsonObj["string"] << std::endl;
    std::cout << "  float: " << jsonObj["float"] << std::endl;

    return 0;
}

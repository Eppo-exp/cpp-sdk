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

// Simple console-based bandit logger
class ConsoleBanditLogger : public eppoclient::BanditLogger {
public:
    void logBanditAction(const eppoclient::BanditEvent& event) override {
        std::cout << "\n=== Bandit Action Log ===" << std::endl;
        std::cout << "Flag Key: " << event.flagKey << std::endl;
        std::cout << "Bandit Key: " << event.banditKey << std::endl;
        std::cout << "Subject: " << event.subject << std::endl;
        std::cout << "Action: " << event.action << std::endl;
        std::cout << "Action Probability: " << event.actionProbability << std::endl;
        std::cout << "Optimality Gap: " << event.optimalityGap << std::endl;
        std::cout << "Model Version: " << event.modelVersion << std::endl;
        std::cout << "Timestamp: " << event.timestamp << std::endl;

        if (!event.subjectNumericAttributes.empty()) {
            std::cout << "Subject Numeric Attributes:" << std::endl;
            for (const auto& [key, value] : event.subjectNumericAttributes) {
                std::cout << "  " << key << ": " << value << std::endl;
            }
        }

        if (!event.subjectCategoricalAttributes.empty()) {
            std::cout << "Subject Categorical Attributes:" << std::endl;
            for (const auto& [key, value] : event.subjectCategoricalAttributes) {
                std::cout << "  " << key << ": " << value << std::endl;
            }
        }

        if (!event.actionNumericAttributes.empty()) {
            std::cout << "Action Numeric Attributes:" << std::endl;
            for (const auto& [key, value] : event.actionNumericAttributes) {
                std::cout << "  " << key << ": " << value << std::endl;
            }
        }

        if (!event.actionCategoricalAttributes.empty()) {
            std::cout << "Action Categorical Attributes:" << std::endl;
            for (const auto& [key, value] : event.actionCategoricalAttributes) {
                std::cout << "  " << key << ": " << value << std::endl;
            }
        }

        if (!event.metaData.empty()) {
            std::cout << "Metadata:" << std::endl;
            for (const auto& [key, value] : event.metaData) {
                std::cout << "  " << key << ": " << value << std::endl;
            }
        }
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

// Helper function to load complete configuration (flags + bandits) from JSON files
bool loadConfiguration(const std::string& flagsFilepath, const std::string& banditsFilepath,
                       eppoclient::Configuration& config) {
    // Read flags configuration file
    std::ifstream flagsFile(flagsFilepath);
    if (!flagsFile.is_open()) {
        std::cerr << "Failed to open flags configuration file: " << flagsFilepath << std::endl;
        return false;
    }
    std::string flagsJson((std::istreambuf_iterator<char>(flagsFile)),
                          std::istreambuf_iterator<char>());

    // Read bandit models file
    std::ifstream banditsFile(banditsFilepath);
    if (!banditsFile.is_open()) {
        std::cerr << "Failed to open bandit models file: " << banditsFilepath << std::endl;
        return false;
    }
    std::string banditsJson((std::istreambuf_iterator<char>(banditsFile)),
                            std::istreambuf_iterator<char>());

    // Parse both configurations at once
    auto result = eppoclient::parseConfiguration(flagsJson, banditsJson);

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
    // Load the bandit flags and models configuration
    std::cout << "Loading bandit flags and models configuration..." << std::endl;
    eppoclient::Configuration config;
    if (!loadConfiguration("config/bandit-flags-v1.json", "config/bandit-models-v1.json", config)) {
        return 1;
    }

    // Create configuration store and set the configuration with both flags and bandits
    auto configStore = std::make_shared<eppoclient::ConfigurationStore>();
    configStore->setConfiguration(config);

    // Create assignment logger, bandit logger, and application logger
    auto assignmentLogger = std::make_shared<ConsoleAssignmentLogger>();
    auto banditLogger = std::make_shared<ConsoleBanditLogger>();
    auto applicationLogger = std::make_shared<ConsoleApplicationLogger>();

    // Create EppoClient with all parameters including bandit logger
    eppoclient::EppoClient client(configStore, assignmentLogger, banditLogger, applicationLogger);

    std::cout << "\n=== Car Bandit: Selecting Car to Recommend ===" << std::endl;

    // Define subject attributes (user/context attributes)
    // Car bandit doesn't require subject attributes, but we can include them
    eppoclient::ContextAttributes subjectAttrs;

    // Define available actions (cars) with their attributes
    std::map<std::string, eppoclient::ContextAttributes> actions;

    // Action 1: Toyota
    // Attributes: speed (numeric)
    eppoclient::ContextAttributes toyotaAction;
    toyotaAction.numericAttributes["speed"] = 120.0;
    actions["toyota"] = toyotaAction;

    // Action 2: Honda (no specific model, but adding for variety)
    eppoclient::ContextAttributes hondaAction;
    hondaAction.numericAttributes["speed"] = 115.0;
    actions["honda"] = hondaAction;

    // Get bandit action recommendation
    eppoclient::BanditResult banditResult =
        client.getBanditAction("car_bandit_flag",  // flag key
                               "user-abc123",      // subject key
                               subjectAttrs,       // subject attributes
                               actions,            // available actions
                               "car_bandit"        // default variation if flag not found
        );

    std::cout << "Bandit selected variation: " << banditResult.variation << std::endl;
    if (banditResult.action.has_value()) {
        std::cout << "Recommended car: " << banditResult.action.value() << std::endl;

        // Use the recommended action in your application logic
        if (banditResult.action.value() == "toyota") {
            std::cout << "✓ Recommending Toyota to user" << std::endl;
        } else if (banditResult.action.value() == "honda") {
            std::cout << "✓ Recommending Honda to user" << std::endl;
        } else {
            std::cout << "✓ Recommending " << banditResult.action.value() << " to user"
                      << std::endl;
        }
    } else {
        std::cout << "No action selected (using default)" << std::endl;
    }

    return 0;
}

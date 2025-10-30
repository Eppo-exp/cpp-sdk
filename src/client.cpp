#include "client.hpp"
#include <stdexcept>

namespace eppoclient {

// ============================================================================
// EppoClient Implementation
// ============================================================================

EppoClient::EppoClient(std::shared_ptr<ConfigurationStore> configStore,
                       std::shared_ptr<AssignmentLogger> assignmentLogger,
                       std::shared_ptr<ApplicationLogger> applicationLogger)
    : configurationStore_(configStore),
      assignmentLogger_(assignmentLogger),
      applicationLogger_(applicationLogger) {}

bool EppoClient::getBoolAssignment(const std::string& flagKey,
                                   const std::string& subjectKey,
                                   const Attributes& subjectAttributes,
                                   bool defaultValue) {
    return getBoolAssignmentInternal(flagKey, subjectKey, subjectAttributes, defaultValue);
}

bool EppoClient::getBoolAssignmentInternal(const std::string& flagKey,
                                           const std::string& subjectKey,
                                           const Attributes& subjectAttributes,
                                           bool defaultValue) {
    try {
        Configuration config = configurationStore_->getConfiguration();
        auto variation = getAssignment(config, flagKey, subjectKey, subjectAttributes, VariationType::BOOLEAN);

        if (!variation.has_value()) {
            return defaultValue;
        }

        // Try to extract bool from variant
        if (std::holds_alternative<bool>(*variation)) {
            return std::get<bool>(*variation);
        }

        // Type mismatch
        if (applicationLogger_) {
            applicationLogger_->error("Failed to cast variation to bool for flag: " + flagKey);
        }
        return defaultValue;

    } catch (const std::exception& e) {
        if (applicationLogger_) {
            applicationLogger_->error(std::string("Error in getBoolAssignment: ") + e.what());
        }
        return defaultValue;
    }
}

double EppoClient::getNumericAssignment(const std::string& flagKey,
                                        const std::string& subjectKey,
                                        const Attributes& subjectAttributes,
                                        double defaultValue) {
    return getNumericAssignmentInternal(flagKey, subjectKey, subjectAttributes, defaultValue);
}

double EppoClient::getNumericAssignmentInternal(const std::string& flagKey,
                                                const std::string& subjectKey,
                                                const Attributes& subjectAttributes,
                                                double defaultValue) {
    try {
        Configuration config = configurationStore_->getConfiguration();
        auto variation = getAssignment(config, flagKey, subjectKey, subjectAttributes, VariationType::NUMERIC);

        if (!variation.has_value()) {
            return defaultValue;
        }

        // Try to extract double from variant
        if (std::holds_alternative<double>(*variation)) {
            return std::get<double>(*variation);
        }

        // Type mismatch
        if (applicationLogger_) {
            applicationLogger_->error("Failed to cast variation to double for flag: " + flagKey);
        }
        return defaultValue;

    } catch (const std::exception& e) {
        if (applicationLogger_) {
            applicationLogger_->error(std::string("Error in getNumericAssignment: ") + e.what());
        }
        return defaultValue;
    }
}

int64_t EppoClient::getIntegerAssignment(const std::string& flagKey,
                                         const std::string& subjectKey,
                                         const Attributes& subjectAttributes,
                                         int64_t defaultValue) {
    return getIntegerAssignmentInternal(flagKey, subjectKey, subjectAttributes, defaultValue);
}

int64_t EppoClient::getIntegerAssignmentInternal(const std::string& flagKey,
                                                 const std::string& subjectKey,
                                                 const Attributes& subjectAttributes,
                                                 int64_t defaultValue) {
    try {
        Configuration config = configurationStore_->getConfiguration();
        auto variation = getAssignment(config, flagKey, subjectKey, subjectAttributes, VariationType::INTEGER);

        if (!variation.has_value()) {
            return defaultValue;
        }

        // Try to extract int64_t from variant
        if (std::holds_alternative<int64_t>(*variation)) {
            return std::get<int64_t>(*variation);
        }

        // Type mismatch
        if (applicationLogger_) {
            applicationLogger_->error("Failed to cast variation to int64_t for flag: " + flagKey);
        }
        return defaultValue;

    } catch (const std::exception& e) {
        if (applicationLogger_) {
            applicationLogger_->error(std::string("Error in getIntegerAssignment: ") + e.what());
        }
        return defaultValue;
    }
}

std::string EppoClient::getStringAssignment(const std::string& flagKey,
                                           const std::string& subjectKey,
                                           const Attributes& subjectAttributes,
                                           const std::string& defaultValue) {
    return getStringAssignmentInternal(flagKey, subjectKey, subjectAttributes, defaultValue);
}

std::string EppoClient::getStringAssignmentInternal(const std::string& flagKey,
                                                    const std::string& subjectKey,
                                                    const Attributes& subjectAttributes,
                                                    const std::string& defaultValue) {
    try {
        Configuration config = configurationStore_->getConfiguration();
        auto variation = getAssignment(config, flagKey, subjectKey, subjectAttributes, VariationType::STRING);

        if (!variation.has_value()) {
            return defaultValue;
        }

        // Try to extract string from variant
        if (std::holds_alternative<std::string>(*variation)) {
            return std::get<std::string>(*variation);
        }

        // Type mismatch
        if (applicationLogger_) {
            applicationLogger_->error("Failed to cast variation to string for flag: " + flagKey);
        }
        return defaultValue;

    } catch (const std::exception& e) {
        if (applicationLogger_) {
            applicationLogger_->error(std::string("Error in getStringAssignment: ") + e.what());
        }
        return defaultValue;
    }
}

nlohmann::json EppoClient::getJSONAssignment(const std::string& flagKey,
                                            const std::string& subjectKey,
                                            const Attributes& subjectAttributes,
                                            const nlohmann::json& defaultValue) {
    return getJSONAssignmentInternal(flagKey, subjectKey, subjectAttributes, defaultValue);
}

nlohmann::json EppoClient::getJSONAssignmentInternal(const std::string& flagKey,
                                                     const std::string& subjectKey,
                                                     const Attributes& subjectAttributes,
                                                     const nlohmann::json& defaultValue) {
    try {
        Configuration config = configurationStore_->getConfiguration();
        auto variation = getAssignment(config, flagKey, subjectKey, subjectAttributes, VariationType::JSON);

        if (!variation.has_value()) {
            return defaultValue;
        }

        // Try to extract json from variant
        if (std::holds_alternative<nlohmann::json>(*variation)) {
            return std::get<nlohmann::json>(*variation);
        }

        // Type mismatch
        if (applicationLogger_) {
            applicationLogger_->error("Failed to cast variation to json for flag: " + flagKey);
        }
        return defaultValue;

    } catch (const std::exception& e) {
        if (applicationLogger_) {
            applicationLogger_->error(std::string("Error in getJSONAssignment: ") + e.what());
        }
        return defaultValue;
    }
}

std::optional<std::variant<std::string, int64_t, double, bool, nlohmann::json>>
EppoClient::getAssignment(const Configuration& config,
                         const std::string& flagKey,
                         const std::string& subjectKey,
                         const Attributes& subjectAttributes,
                         VariationType variationType) {
    // Validate inputs
    if (subjectKey.empty()) {
        if (applicationLogger_) {
            applicationLogger_->error("No subject key provided");
        }
        throw std::invalid_argument("no subject key provided");
    }

    if (flagKey.empty()) {
        if (applicationLogger_) {
            applicationLogger_->error("No flag key provided");
        }
        throw std::invalid_argument("no flag key provided");
    }

    // Get flag configuration
    const FlagConfiguration* flag = config.getFlagConfiguration(flagKey);
    if (flag == nullptr) {
        if (applicationLogger_) {
            applicationLogger_->info("Failed to get flag configuration for: " + flagKey);
        }
        throw FlagConfigurationNotFoundException();
    }

    // Verify flag type
    try {
        verifyType(*flag, variationType);
    } catch (const std::exception& e) {
        if (applicationLogger_) {
            applicationLogger_->warn(std::string("Failed to verify flag type: ") + e.what());
        }
        throw;
    }

    // Evaluate flag
    EvalResult result;
    try {
        result = evalFlag(*flag, subjectKey, subjectAttributes, applicationLogger_.get());
    } catch (const std::exception& e) {
        if (applicationLogger_) {
            applicationLogger_->error(std::string("Failed to evaluate flag: ") + e.what());
        }
        throw;
    }

    // Log assignment event
    logAssignment(result.event);

    return result.value;
}

void EppoClient::logAssignment(const std::optional<AssignmentEvent>& event) {
    if (!event.has_value() || !assignmentLogger_) {
        return;
    }

    try {
        assignmentLogger_->logAssignment(*event);
    } catch (const std::exception& e) {
        if (applicationLogger_) {
            applicationLogger_->error(std::string("Error logging assignment: ") + e.what());
        }
    } catch (...) {
        if (applicationLogger_) {
            applicationLogger_->error("Unknown error logging assignment");
        }
    }
}

} // namespace eppoclient

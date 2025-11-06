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
      applicationLogger_(applicationLogger ? applicationLogger : std::make_shared<NoOpApplicationLogger>()) {}

bool EppoClient::getBoolAssignment(const std::string& flagKey,
                                   const std::string& subjectKey,
                                   const Attributes& subjectAttributes,
                                   bool defaultValue) {
    try {
        Configuration config = configurationStore_->getConfiguration();
        auto variation = getAssignment(config, flagKey, subjectKey, subjectAttributes, VariationType::BOOLEAN);
        return extractVariation(variation, flagKey, VariationType::BOOLEAN, defaultValue);
    } catch (const std::exception& e) {
        applicationLogger_->error(std::string("Error in getBoolAssignment: ") + e.what());
        return defaultValue;
    }
}

double EppoClient::getNumericAssignment(const std::string& flagKey,
                                        const std::string& subjectKey,
                                        const Attributes& subjectAttributes,
                                        double defaultValue) {
    try {
        Configuration config = configurationStore_->getConfiguration();
        auto variation = getAssignment(config, flagKey, subjectKey, subjectAttributes, VariationType::NUMERIC);
        return extractVariation(variation, flagKey, VariationType::NUMERIC, defaultValue);
    } catch (const std::exception& e) {
        applicationLogger_->error(std::string("Error in getNumericAssignment: ") + e.what());
        return defaultValue;
    }
}

int64_t EppoClient::getIntegerAssignment(const std::string& flagKey,
                                         const std::string& subjectKey,
                                         const Attributes& subjectAttributes,
                                         int64_t defaultValue) {
    try {
        Configuration config = configurationStore_->getConfiguration();
        auto variation = getAssignment(config, flagKey, subjectKey, subjectAttributes, VariationType::INTEGER);
        return extractVariation(variation, flagKey, VariationType::INTEGER, defaultValue);
    } catch (const std::exception& e) {
        applicationLogger_->error(std::string("Error in getIntegerAssignment: ") + e.what());
        return defaultValue;
    }
}

std::string EppoClient::getStringAssignment(const std::string& flagKey,
                                           const std::string& subjectKey,
                                           const Attributes& subjectAttributes,
                                           const std::string& defaultValue) {
    try {
        Configuration config = configurationStore_->getConfiguration();
        auto variation = getAssignment(config, flagKey, subjectKey, subjectAttributes, VariationType::STRING);
        return extractVariation(variation, flagKey, VariationType::STRING, defaultValue);
    } catch (const std::exception& e) {
        applicationLogger_->error(std::string("Error in getStringAssignment: ") + e.what());
        return defaultValue;
    }
}

nlohmann::json EppoClient::getJSONAssignment(const std::string& flagKey,
                                            const std::string& subjectKey,
                                            const Attributes& subjectAttributes,
                                            const nlohmann::json& defaultValue) {
    try {
        Configuration config = configurationStore_->getConfiguration();
        auto variation = getAssignment(config, flagKey, subjectKey, subjectAttributes, VariationType::JSON);
        return extractVariation(variation, flagKey, VariationType::JSON, defaultValue);
    } catch (const std::exception& e) {
        applicationLogger_->error(std::string("Error in getJSONAssignment: ") + e.what());
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
        applicationLogger_->error("No subject key provided");
        throw std::invalid_argument("no subject key provided");
    }

    if (flagKey.empty()) {
        applicationLogger_->error("No flag key provided");
        throw std::invalid_argument("no flag key provided");
    }

    // Get flag configuration
    const FlagConfiguration* flag = config.getFlagConfiguration(flagKey);
    if (flag == nullptr) {
        applicationLogger_->info("Failed to get flag configuration for: " + flagKey);
        throw FlagConfigurationNotFoundException();
    }

    // Verify flag type
    try {
        verifyType(*flag, variationType);
    } catch (const std::exception& e) {
        applicationLogger_->warn(std::string("Failed to verify flag type: ") + e.what());
        throw;
    }

    // Evaluate flag
    EvalResult result;
    try {
        result = evalFlag(*flag, subjectKey, subjectAttributes, applicationLogger_.get());
    } catch (const std::exception& e) {
        applicationLogger_->error(std::string("Failed to evaluate flag: ") + e.what());
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
        applicationLogger_->error(std::string("Error logging assignment: ") + e.what());
    } catch (...) {
        applicationLogger_->error("Unknown error logging assignment");
    }
}

} // namespace eppoclient

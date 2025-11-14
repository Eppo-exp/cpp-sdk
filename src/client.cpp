#include "client.hpp"
#include "src/version.hpp"
#include "time_utils.hpp"
#include <stdexcept>
#include <chrono>

namespace eppoclient {

// ============================================================================
// EppoClient Implementation
// ============================================================================

EppoClient::EppoClient(std::shared_ptr<ConfigurationStore> configStore,
                       std::shared_ptr<AssignmentLogger> assignmentLogger,
                       std::shared_ptr<BanditLogger> banditLogger,
                       std::shared_ptr<ApplicationLogger> applicationLogger)
    : configurationStore_(configStore),
      assignmentLogger_(assignmentLogger),
      banditLogger_(banditLogger),
      applicationLogger_(applicationLogger ? applicationLogger : std::make_shared<NoOpApplicationLogger>()),
      isGracefulFailureMode_(true) {}

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
        if (!isGracefulFailureMode_) {
            throw;
        }
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
        if (!isGracefulFailureMode_) {
            throw;
        }
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
        if (!isGracefulFailureMode_) {
            throw;
        }
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
        if (!isGracefulFailureMode_) {
            throw;
        }
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
        if (!isGracefulFailureMode_) {
            throw;
        }
        return defaultValue;
    }
}

std::string EppoClient::getSerializedJSONAssignment(const std::string& flagKey,
                                                   const std::string& subjectKey,
                                                   const Attributes& subjectAttributes,
                                                   const std::string& defaultValue) {
    try {
        Configuration config = configurationStore_->getConfiguration();
        auto variation = getAssignment(config, flagKey, subjectKey, subjectAttributes, VariationType::JSON);

        if (!variation.has_value()) {
            return defaultValue;
        }

        if (!std::holds_alternative<nlohmann::json>(*variation)) {
            std::string actualType = detectVariationType(*variation);
            std::string expectedType = variationTypeToString(VariationType::JSON);
            applicationLogger_->error("Variation value does not have the correct type. Found " + actualType +
                                    ", but expected " + expectedType + " for flag " + flagKey);
            return defaultValue;
        }

        return std::get<nlohmann::json>(*variation).dump();
    } catch (const std::exception& e) {
        applicationLogger_->error(std::string("Error in getSerializedJSONAssignment: ") + e.what());
        if (!isGracefulFailureMode_) {
            throw;
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

void EppoClient::logBanditAction(const BanditEvent& event) {
    if (!banditLogger_) {
        return;
    }

    try {
        banditLogger_->logBanditAction(event);
    } catch (const std::exception& e) {
        if (applicationLogger_) {
            applicationLogger_->error(std::string("Error logging bandit action: ") + e.what());
        }
    } catch (...) {
        if (applicationLogger_) {
            applicationLogger_->error("Unknown error logging bandit action");
        }
    }
}

BanditResult EppoClient::getBanditAction(const std::string& flagKey,
                                        const std::string& subjectKey,
                                        const ContextAttributes& subjectAttributes,
                                        const std::map<std::string, ContextAttributes>& actions,
                                        const std::string& defaultVariation) {
    return getBanditActionInternal(flagKey, subjectKey, subjectAttributes, actions, defaultVariation);
}

BanditResult EppoClient::getBanditActionInternal(const std::string& flagKey,
                                                 const std::string& subjectKey,
                                                 const ContextAttributes& subjectAttributes,
                                                 const std::map<std::string, ContextAttributes>& actions,
                                                 const std::string& defaultVariation) {
    Configuration config = configurationStore_->getConfiguration();

    // Ignoring the error here as we can always proceed with default variation
    std::string variation = defaultVariation;
    try {
        auto assignmentValue = getAssignment(config, flagKey, subjectKey,
                                            toGenericAttributes(subjectAttributes),
                                            VariationType::STRING);
        if (assignmentValue.has_value() && std::holds_alternative<std::string>(*assignmentValue)) {
            variation = std::get<std::string>(*assignmentValue);
        }
    } catch (...) {
        // Silently ignore errors and use default variation
    }

    // If no actions have been passed, return the variation with no action
    if (actions.empty()) {
        return BanditResult(variation, std::nullopt);
    }

    // Get bandit variation
    BanditVariation banditVariation;
    if (!config.getBanditVariant(flagKey, variation, banditVariation)) {
        return BanditResult(variation, std::nullopt);
    }

    // Get bandit configuration
    const BanditConfiguration* bandit = config.getBanditConfiguration(banditVariation.key);
    if (bandit == nullptr) {
        return BanditResult(variation, std::nullopt);
    }

    // Evaluate bandit
    BanditEvaluationContext evalContext;
    evalContext.flagKey = flagKey;
    evalContext.subjectKey = subjectKey;
    evalContext.subjectAttributes = subjectAttributes;
    evalContext.actions = actions;

    BanditEvaluationDetails evaluation = evaluateBandit(bandit->modelData, evalContext);

    // Log bandit action
    BanditEvent event;
    event.flagKey = flagKey;
    event.banditKey = bandit->banditKey;
    event.subject = subjectKey;
    event.action = evaluation.actionKey;
    event.actionProbability = evaluation.actionWeight;
    event.optimalityGap = evaluation.optimalityGap;
    event.modelVersion = bandit->modelVersion;
    event.timestamp = formatISOTimestamp(std::chrono::system_clock::now());
    event.subjectNumericAttributes = evaluation.subjectAttributes.numericAttributes;
    event.subjectCategoricalAttributes = evaluation.subjectAttributes.categoricalAttributes;
    event.actionNumericAttributes = evaluation.actionAttributes.numericAttributes;
    event.actionCategoricalAttributes = evaluation.actionAttributes.categoricalAttributes;
    event.metaData["sdkLanguage"] = "cpp";
    event.metaData["sdkVersion"] = getVersion();

    logBanditAction(event);

    return BanditResult(variation, evaluation.actionKey);
}

void EppoClient::setIsGracefulFailureMode(bool isGracefulFailureMode) {
    isGracefulFailureMode_ = isGracefulFailureMode;
}

} // namespace eppoclient

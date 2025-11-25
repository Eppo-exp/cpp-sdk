#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <memory>
#include <optional>
#include <string>
#include "application_logger.hpp"
#include "config_response.hpp"
#include "configuration.hpp"
#include "configuration_store.hpp"
#include "evalbandits.hpp"
#include "evalflags.hpp"
#include "rules.hpp"

namespace eppoclient {

// EvaluationResult structure to hold variation and evaluation details
template <typename T>
struct EvaluationResult {
    T variation;
    std::optional<std::string> action;
    std::optional<EvaluationDetails> evaluationDetails;

    EvaluationResult(const T& var, std::optional<std::string> act = std::nullopt,
                     std::optional<EvaluationDetails> details = std::nullopt)
        : variation(var), action(act), evaluationDetails(details) {}
};

// Assignment logger interface
class AssignmentLogger {
public:
    virtual ~AssignmentLogger() = default;
    virtual void logAssignment(const AssignmentEvent& event) = 0;
};

// Bandit logger interface
class BanditLogger {
public:
    virtual ~BanditLogger() = default;
    virtual void logBanditAction(const BanditEvent& event) = 0;
};

/**
 * EppoClient - Main client for feature flag evaluation
 *
 * This SDK is built with -fno-exceptions and does not throw exceptions.
 * When errors occur (missing flags, invalid parameters, type mismatches),
 * the SDK:
 *   1. Logs the error through the ApplicationLogger interface
 *   2. Returns the default value provided by the caller
 *
 * This design ensures your application continues running even if flag
 * evaluation fails, making it suitable for production environments and
 * projects that don't use exceptions.
 *
 * Example usage:
 * @code
 * eppoclient::ConfigurationStore configStore;
 * configStore.setConfiguration(config);
 *
 * auto logger = std::make_shared<MyApplicationLogger>();
 * eppoclient::EppoClient client(configStore, nullptr, nullptr, logger);
 *
 * // If flag doesn't exist, logs an info message and returns false
 * bool result = client.getBoolAssignment("my-flag", "user-123", attrs, false);
 * @endcode
 */
class EppoClient {
private:
    ConfigurationStore& configurationStore_;
    std::shared_ptr<AssignmentLogger> assignmentLogger_;
    std::shared_ptr<BanditLogger> banditLogger_;
    std::shared_ptr<ApplicationLogger> applicationLogger_;

    // Internal method to get assignment value
    std::optional<std::variant<std::string, int64_t, double, bool, nlohmann::json>> getAssignment(
        const Configuration& config, const std::string& flagKey, const std::string& subjectKey,
        const Attributes& subjectAttributes, VariationType variationType);

    // Internal method to log assignment
    void logAssignment(const std::optional<AssignmentEvent>& event);

    // Internal method to log bandit action
    void logBanditAction(const BanditEvent& event);


    // Template helper to extract and validate variation value
    template <typename T>
    T extractVariation(
        const std::optional<std::variant<std::string, int64_t, double, bool, nlohmann::json>>&
            variation,
        const std::string& flagKey, VariationType variationType, const T& defaultValue) {
        if (!variation.has_value()) {
            return defaultValue;
        }

        if (!std::holds_alternative<T>(*variation)) {
            std::string actualType = detectVariationType(*variation);
            std::string expectedType = variationTypeToString(variationType);
            applicationLogger_->error("Variation value does not have the correct type. Found " +
                                      actualType + ", but expected " + expectedType + " for flag " +
                                      flagKey);
            return defaultValue;
        }

        return std::get<T>(*variation);
    }

    // Template helper to create error result with consistent error handling
    template <typename T>
    EvaluationResult<T> createErrorResult(const T& defaultValue, const std::string& flagKey,
                                          const std::string& subjectKey,
                                          const Attributes& subjectAttributes,
                                          FlagEvaluationCode errorCode,
                                          const std::string& errorDescription) {
        EvaluationDetails details;
        details.flagKey = flagKey;
        details.subjectKey = subjectKey;
        details.subjectAttributes = subjectAttributes;
        details.flagEvaluationCode = errorCode;
        details.flagEvaluationDescription = errorDescription;

        return EvaluationResult<T>(defaultValue, std::nullopt, details);
    }

public:
    // Constructor
    EppoClient(ConfigurationStore& configStore,
               std::shared_ptr<AssignmentLogger> assignmentLogger = nullptr,
               std::shared_ptr<BanditLogger> banditLogger = nullptr,
               std::shared_ptr<ApplicationLogger> applicationLogger = nullptr);

    // Get boolean assignment
    bool getBoolAssignment(const std::string& flagKey, const std::string& subjectKey,
                           const Attributes& subjectAttributes, bool defaultValue);

    // Get numeric assignment
    double getNumericAssignment(const std::string& flagKey, const std::string& subjectKey,
                                const Attributes& subjectAttributes, double defaultValue);

    // Get integer assignment
    int64_t getIntegerAssignment(const std::string& flagKey, const std::string& subjectKey,
                                 const Attributes& subjectAttributes, int64_t defaultValue);

    // Get string assignment
    std::string getStringAssignment(const std::string& flagKey, const std::string& subjectKey,
                                    const Attributes& subjectAttributes,
                                    const std::string& defaultValue);

    // Get JSON assignment
    nlohmann::json getJSONAssignment(const std::string& flagKey, const std::string& subjectKey,
                                     const Attributes& subjectAttributes,
                                     const nlohmann::json& defaultValue);

    // Get serialized JSON assignment (returns stringified JSON)
    std::string getSerializedJSONAssignment(const std::string& flagKey,
                                            const std::string& subjectKey,
                                            const Attributes& subjectAttributes,
                                            const std::string& defaultValue);

    // Get bandit action
    BanditResult getBanditAction(const std::string& flagKey, const std::string& subjectKey,
                                 const ContextAttributes& subjectAttributes,
                                 const std::map<std::string, ContextAttributes>& actions,
                                 const std::string& defaultVariation);

    // ========== Assignment Details Methods ==========

    // Get boolean assignment with details
    EvaluationResult<bool> getBooleanAssignmentDetails(const std::string& flagKey,
                                                       const std::string& subjectKey,
                                                       const Attributes& subjectAttributes,
                                                       bool defaultValue);

    // Get integer assignment with details
    EvaluationResult<int64_t> getIntegerAssignmentDetails(const std::string& flagKey,
                                                          const std::string& subjectKey,
                                                          const Attributes& subjectAttributes,
                                                          int64_t defaultValue);

    // Get numeric assignment with details
    EvaluationResult<double> getNumericAssignmentDetails(const std::string& flagKey,
                                                         const std::string& subjectKey,
                                                         const Attributes& subjectAttributes,
                                                         double defaultValue);

    // Get string assignment with details
    EvaluationResult<std::string> getStringAssignmentDetails(const std::string& flagKey,
                                                             const std::string& subjectKey,
                                                             const Attributes& subjectAttributes,
                                                             const std::string& defaultValue);

    // Get JSON assignment with details
    EvaluationResult<nlohmann::json> getJsonAssignmentDetails(const std::string& flagKey,
                                                              const std::string& subjectKey,
                                                              const Attributes& subjectAttributes,
                                                              const nlohmann::json& defaultValue);

    // Get serialized JSON assignment with details
    EvaluationResult<std::string> getSerializedJsonAssignmentDetails(
        const std::string& flagKey, const std::string& subjectKey,
        const Attributes& subjectAttributes, const std::string& defaultValue);

    // Get bandit action with details
    EvaluationResult<std::string> getBanditActionDetails(
        const std::string& flagKey, const std::string& subjectKey,
        const ContextAttributes& subjectAttributes,
        const std::map<std::string, ContextAttributes>& actions,
        const std::string& defaultVariation);

    // Generic get assignment details (for advanced use cases)
    template <typename T>
    EvaluationResult<T> getAssignmentDetails(VariationType variationType,
                                             const std::string& flagKey,
                                             const std::string& subjectKey,
                                             const Attributes& subjectAttributes,
                                             const T& defaultValue);

    // Get configuration store
    ConfigurationStore& getConfigurationStore() const { return configurationStore_; }
};

// Template method implementation
template <typename T>
EvaluationResult<T> EppoClient::getAssignmentDetails(VariationType variationType,
                                                     const std::string& flagKey,
                                                     const std::string& subjectKey,
                                                     const Attributes& subjectAttributes,
                                                     const T& defaultValue) {
    // Validate inputs
    if (subjectKey.empty()) {
        applicationLogger_->error("No subject key provided");
        return createErrorResult<T>(defaultValue, flagKey, subjectKey, subjectAttributes,
                                    FlagEvaluationCode::ASSIGNMENT_ERROR,
                                    "No subject key provided");
    }

    if (flagKey.empty()) {
        applicationLogger_->error("No flag key provided");
        return createErrorResult<T>(defaultValue, flagKey, subjectKey, subjectAttributes,
                                    FlagEvaluationCode::ASSIGNMENT_ERROR, "No flag key provided");
    }

    auto config = configurationStore_.getConfiguration();

    // Get flag configuration
    const FlagConfiguration* flag = config->getFlagConfiguration(flagKey);
    if (flag == nullptr) {
        applicationLogger_->info("Failed to get flag configuration for: " + flagKey);
        return createErrorResult<T>(defaultValue, flagKey, subjectKey, subjectAttributes,
                                    FlagEvaluationCode::FLAG_UNRECOGNIZED_OR_DISABLED,
                                    "Flag configuration not found");
    }

    // Verify flag type
    if (!verifyType(*flag, variationType)) {
        applicationLogger_->warn("Failed to verify flag type for: " + flagKey +
                                 " (expected: " + variationTypeToString(variationType) +
                                 ", actual: " + variationTypeToString(flag->variationType) + ")");
        return createErrorResult<T>(defaultValue, flagKey, subjectKey, subjectAttributes,
                                    FlagEvaluationCode::TYPE_MISMATCH, "Type mismatch");
    }

    // Evaluate flag with details
    EvalResultWithDetails result =
        evalFlagDetails(*flag, subjectKey, subjectAttributes, applicationLogger_.get());

    // Log assignment event
    logAssignment(result.event);

    // Check if there was an error
    if (result.details.flagEvaluationCode.has_value() &&
        *result.details.flagEvaluationCode != FlagEvaluationCode::MATCH) {
        // Return default with details
        return EvaluationResult<T>(defaultValue, std::nullopt, result.details);
    }

    if (!result.value.has_value()) {
        return EvaluationResult<T>(defaultValue, std::nullopt, result.details);
    }

    if (!std::holds_alternative<T>(*result.value)) {
        std::string actualType = detectVariationType(*result.value);
        std::string expectedType = variationTypeToString(variationType);
        applicationLogger_->error("Variation value does not have the correct type. Found " +
                                  actualType + ", but expected " + expectedType + " for flag " +
                                  flagKey);
        return EvaluationResult<T>(defaultValue, std::nullopt, result.details);
    }

    return EvaluationResult<T>(std::get<T>(*result.value), std::nullopt, result.details);
}

}  // namespace eppoclient

#endif  // CLIENT_H

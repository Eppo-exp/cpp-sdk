#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <memory>
#include <string>
#include "application_logger.hpp"
#include "config_response.hpp"
#include "configuration_store.hpp"
#include "evalbandits.hpp"
#include "evalflags.hpp"
#include "evaluation_client.hpp"
#include "rules.hpp"

namespace eppoclient {

// ============================================================================
// NoOp Logger Implementations
// ============================================================================
// Note: EvaluationResult, AssignmentLogger, and BanditLogger are defined in evaluation_client.hpp

// No-op implementation of AssignmentLogger (does nothing)
class NoOpAssignmentLogger : public AssignmentLogger {
public:
    void logAssignment(const AssignmentEvent&) override {
        // No-op: do nothing
    }
};

// No-op implementation of BanditLogger (does nothing)
class NoOpBanditLogger : public BanditLogger {
public:
    void logBanditAction(const BanditEvent&) override {
        // No-op: do nothing
    }
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
 * auto configStore = std::make_shared<eppoclient::ConfigurationStore>(config);
 *
 * auto logger = std::make_shared<MyApplicationLogger>();
 * eppoclient::EppoClient client(configStore, nullptr, nullptr, logger);
 *
 * // If flag doesn't exist, logs an info message and returns false
 * bool result = client.getBooleanAssignment("my-flag", "user-123", attrs, false);
 * @endcode
 */
class EppoClient {
private:
    std::shared_ptr<ConfigurationStore> configurationStore_;
    std::shared_ptr<AssignmentLogger> assignmentLogger_;
    std::shared_ptr<BanditLogger> banditLogger_;
    std::shared_ptr<ApplicationLogger> applicationLogger_;

    // Helper method to create EvaluationClient instance with given configuration
    EvaluationClient evaluationClient(const Configuration& config) const;

public:
    // Constructor
    EppoClient(std::shared_ptr<ConfigurationStore> configStore,
               std::shared_ptr<AssignmentLogger> assignmentLogger = nullptr,
               std::shared_ptr<BanditLogger> banditLogger = nullptr,
               std::shared_ptr<ApplicationLogger> applicationLogger = nullptr);

    // Get boolean assignment
    bool getBooleanAssignment(const std::string& flagKey, const std::string& subjectKey,
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
    ConfigurationStore& getConfigurationStore() const { return *configurationStore_; }
};

// Template method implementation
template <typename T>
EvaluationResult<T> EppoClient::getAssignmentDetails(VariationType variationType,
                                                     const std::string& flagKey,
                                                     const std::string& subjectKey,
                                                     const Attributes& subjectAttributes,
                                                     const T& defaultValue) {
    auto config = configurationStore_->getConfiguration();
    return evaluationClient(*config).getAssignmentDetails<T>(variationType, flagKey, subjectKey,
                                                             subjectAttributes, defaultValue);
}

}  // namespace eppoclient

#endif  // CLIENT_H

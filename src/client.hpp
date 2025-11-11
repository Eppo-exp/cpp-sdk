#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <memory>
#include <optional>
#include <stdexcept>
#include "application_logger.hpp"
#include "config_response.hpp"
#include "rules.hpp"
#include "evalflags.hpp"
#include "configuration.hpp"
#include "configuration_store.hpp"
#include "evalbandits.hpp"

namespace eppoclient {

// Error exception for missing flag configuration
class FlagConfigurationNotFoundException : public std::runtime_error {
public:
    FlagConfigurationNotFoundException()
        : std::runtime_error("flag configuration not found") {}
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

// EppoClient - Main client for feature flag evaluation
class EppoClient {
private:
    std::shared_ptr<ConfigurationStore> configurationStore_;
    std::shared_ptr<AssignmentLogger> assignmentLogger_;
    std::shared_ptr<BanditLogger> banditLogger_;
    std::shared_ptr<ApplicationLogger> applicationLogger_;
    bool isGracefulFailureMode_;

    // Internal method to get assignment value
    std::optional<std::variant<std::string, int64_t, double, bool, nlohmann::json>>
    getAssignment(const Configuration& config,
                 const std::string& flagKey,
                 const std::string& subjectKey,
                 const Attributes& subjectAttributes,
                 VariationType variationType);

    // Internal method to log assignment
    void logAssignment(const std::optional<AssignmentEvent>& event);

    // Internal method to log bandit action
    void logBanditAction(const BanditEvent& event);

    
    // Template helper to extract and validate variation value
    template<typename T>
    T extractVariation(const std::optional<std::variant<std::string, int64_t, double, bool, nlohmann::json>>& variation,
                       const std::string& flagKey,
                       VariationType variationType,
                       const T& defaultValue) {
        if (!variation.has_value()) {
            return defaultValue;
        }

        if (!std::holds_alternative<T>(*variation)) {
            std::string actualType = detectVariationType(*variation);
            std::string expectedType = variationTypeToString(variationType);
            applicationLogger_->error("Variation value does not have the correct type. Found " + actualType +
                                    ", but expected " + expectedType + " for flag " + flagKey);
            return defaultValue;
        }

        return std::get<T>(*variation);
    }

    // Internal method for bandit action
    BanditResult getBanditActionInternal(const std::string& flagKey,
                                        const std::string& subjectKey,
                                        const ContextAttributes& subjectAttributes,
                                        const std::map<std::string, ContextAttributes>& actions,
                                        const std::string& defaultVariation);

public:
    // Constructor
    EppoClient(std::shared_ptr<ConfigurationStore> configStore,
              std::shared_ptr<AssignmentLogger> assignmentLogger = nullptr,
              std::shared_ptr<BanditLogger> banditLogger = nullptr,
              std::shared_ptr<ApplicationLogger> applicationLogger = nullptr);

    // Get boolean assignment
    bool getBoolAssignment(const std::string& flagKey,
                          const std::string& subjectKey,
                          const Attributes& subjectAttributes,
                          bool defaultValue);

    // Get numeric assignment
    double getNumericAssignment(const std::string& flagKey,
                               const std::string& subjectKey,
                               const Attributes& subjectAttributes,
                               double defaultValue);

    // Get integer assignment
    int64_t getIntegerAssignment(const std::string& flagKey,
                                const std::string& subjectKey,
                                const Attributes& subjectAttributes,
                                int64_t defaultValue);

    // Get string assignment
    std::string getStringAssignment(const std::string& flagKey,
                                   const std::string& subjectKey,
                                   const Attributes& subjectAttributes,
                                   const std::string& defaultValue);

    // Get JSON assignment
    nlohmann::json getJSONAssignment(const std::string& flagKey,
                                    const std::string& subjectKey,
                                    const Attributes& subjectAttributes,
                                    const nlohmann::json& defaultValue);

                                        // Get serialized JSON assignment (returns stringified JSON)
    std::string getSerializedJSONAssignment(const std::string& flagKey,
        const std::string& subjectKey,
        const Attributes& subjectAttributes,
        const std::string& defaultValue);

    // Get bandit action
    BanditResult getBanditAction(const std::string& flagKey,
                                const std::string& subjectKey,
                                const ContextAttributes& subjectAttributes,
                                const std::map<std::string, ContextAttributes>& actions,
                                const std::string& defaultVariation);

    // Set graceful failure mode
    void setIsGracefulFailureMode(bool isGracefulFailureMode);

    // Get configuration store
    std::shared_ptr<ConfigurationStore> getConfigurationStore() const {
        return configurationStore_;
    }
};

} // namespace eppoclient

#endif // CLIENT_H

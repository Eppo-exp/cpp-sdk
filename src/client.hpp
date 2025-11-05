#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <memory>
#include <optional>
#include <stdexcept>
#include "config_response.hpp"
#include "rules.hpp"
#include "evalflags.hpp"
#include "configuration.hpp"
#include "configuration_store.hpp"

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

// EppoClient - Main client for feature flag evaluation
class EppoClient {
private:
    std::shared_ptr<ConfigurationStore> configurationStore_;
    std::shared_ptr<AssignmentLogger> assignmentLogger_;
    std::shared_ptr<ApplicationLogger> applicationLogger_;

    // Internal method to get assignment value
    std::optional<std::variant<std::string, int64_t, double, bool, nlohmann::json>>
    getAssignment(const Configuration& config,
                 const std::string& flagKey,
                 const std::string& subjectKey,
                 const Attributes& subjectAttributes,
                 VariationType variationType);

    // Internal method to log assignment
    void logAssignment(const std::optional<AssignmentEvent>& event);

    // Internal method for boolean assignment
    bool getBoolAssignmentInternal(const std::string& flagKey,
                                   const std::string& subjectKey,
                                   const Attributes& subjectAttributes,
                                   bool defaultValue);

    // Internal method for numeric assignment
    double getNumericAssignmentInternal(const std::string& flagKey,
                                       const std::string& subjectKey,
                                       const Attributes& subjectAttributes,
                                       double defaultValue);

    // Internal method for integer assignment
    int64_t getIntegerAssignmentInternal(const std::string& flagKey,
                                        const std::string& subjectKey,
                                        const Attributes& subjectAttributes,
                                        int64_t defaultValue);

    // Internal method for string assignment
    std::string getStringAssignmentInternal(const std::string& flagKey,
                                           const std::string& subjectKey,
                                           const Attributes& subjectAttributes,
                                           const std::string& defaultValue);

    // Internal method for JSON assignment
    nlohmann::json getJSONAssignmentInternal(const std::string& flagKey,
                                            const std::string& subjectKey,
                                            const Attributes& subjectAttributes,
                                            const nlohmann::json& defaultValue);

public:
    // Constructor
    EppoClient(std::shared_ptr<ConfigurationStore> configStore,
              std::shared_ptr<AssignmentLogger> assignmentLogger = nullptr,
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

    // Get configuration store
    std::shared_ptr<ConfigurationStore> getConfigurationStore() const {
        return configurationStore_;
    }
};

} // namespace eppoclient

#endif // CLIENT_H

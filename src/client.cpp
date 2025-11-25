#include "client.hpp"

namespace eppoclient {

// ============================================================================
// EppoClient Implementation
// ============================================================================

EppoClient::EppoClient(ConfigurationStore& configStore,
                       std::shared_ptr<AssignmentLogger> assignmentLogger,
                       std::shared_ptr<BanditLogger> banditLogger,
                       std::shared_ptr<ApplicationLogger> applicationLogger)
    : configurationStore_(configStore),
      assignmentLogger_(assignmentLogger ? assignmentLogger
                                         : std::make_shared<NoOpAssignmentLogger>()),
      banditLogger_(banditLogger ? banditLogger : std::make_shared<NoOpBanditLogger>()),
      applicationLogger_(applicationLogger ? applicationLogger
                                           : std::make_shared<NoOpApplicationLogger>()) {}

EvaluationClient EppoClient::evaluationClient(const Configuration& config) const {
    return EvaluationClient(config, *assignmentLogger_, *banditLogger_, *applicationLogger_);
}

bool EppoClient::getBoolAssignment(const std::string& flagKey, const std::string& subjectKey,
                                   const Attributes& subjectAttributes, bool defaultValue) {
    auto config = configurationStore_.getConfiguration();
    return evaluationClient(*config).getBoolAssignment(flagKey, subjectKey, subjectAttributes,
                                                       defaultValue);
}

double EppoClient::getNumericAssignment(const std::string& flagKey, const std::string& subjectKey,
                                        const Attributes& subjectAttributes, double defaultValue) {
    auto config = configurationStore_.getConfiguration();
    return evaluationClient(*config).getNumericAssignment(flagKey, subjectKey, subjectAttributes,
                                                          defaultValue);
}

int64_t EppoClient::getIntegerAssignment(const std::string& flagKey, const std::string& subjectKey,
                                         const Attributes& subjectAttributes,
                                         int64_t defaultValue) {
    auto config = configurationStore_.getConfiguration();
    return evaluationClient(*config).getIntegerAssignment(flagKey, subjectKey, subjectAttributes,
                                                          defaultValue);
}

std::string EppoClient::getStringAssignment(const std::string& flagKey,
                                            const std::string& subjectKey,
                                            const Attributes& subjectAttributes,
                                            const std::string& defaultValue) {
    auto config = configurationStore_.getConfiguration();
    return evaluationClient(*config).getStringAssignment(flagKey, subjectKey, subjectAttributes,
                                                         defaultValue);
}

nlohmann::json EppoClient::getJSONAssignment(const std::string& flagKey,
                                             const std::string& subjectKey,
                                             const Attributes& subjectAttributes,
                                             const nlohmann::json& defaultValue) {
    auto config = configurationStore_.getConfiguration();
    return evaluationClient(*config).getJSONAssignment(flagKey, subjectKey, subjectAttributes,
                                                       defaultValue);
}

std::string EppoClient::getSerializedJSONAssignment(const std::string& flagKey,
                                                    const std::string& subjectKey,
                                                    const Attributes& subjectAttributes,
                                                    const std::string& defaultValue) {
    auto config = configurationStore_.getConfiguration();
    return evaluationClient(*config).getSerializedJSONAssignment(flagKey, subjectKey,
                                                                 subjectAttributes, defaultValue);
}

BanditResult EppoClient::getBanditAction(const std::string& flagKey, const std::string& subjectKey,
                                         const ContextAttributes& subjectAttributes,
                                         const std::map<std::string, ContextAttributes>& actions,
                                         const std::string& defaultVariation) {
    auto config = configurationStore_.getConfiguration();
    return evaluationClient(*config).getBanditAction(flagKey, subjectKey, subjectAttributes,
                                                     actions, defaultVariation);
}

EvaluationResult<std::string> EppoClient::getBanditActionDetails(
    const std::string& flagKey, const std::string& subjectKey,
    const ContextAttributes& subjectAttributes,
    const std::map<std::string, ContextAttributes>& actions, const std::string& defaultVariation) {
    auto config = configurationStore_.getConfiguration();
    return evaluationClient(*config).getBanditActionDetails(flagKey, subjectKey, subjectAttributes,
                                                            actions, defaultVariation);
}

// ============================================================================
// Assignment Details Methods
// ============================================================================

EvaluationResult<bool> EppoClient::getBooleanAssignmentDetails(const std::string& flagKey,
                                                               const std::string& subjectKey,
                                                               const Attributes& subjectAttributes,
                                                               bool defaultValue) {
    auto config = configurationStore_.getConfiguration();
    return evaluationClient(*config).getBooleanAssignmentDetails(flagKey, subjectKey,
                                                                 subjectAttributes, defaultValue);
}

EvaluationResult<int64_t> EppoClient::getIntegerAssignmentDetails(
    const std::string& flagKey, const std::string& subjectKey, const Attributes& subjectAttributes,
    int64_t defaultValue) {
    auto config = configurationStore_.getConfiguration();
    return evaluationClient(*config).getIntegerAssignmentDetails(flagKey, subjectKey,
                                                                 subjectAttributes, defaultValue);
}

EvaluationResult<double> EppoClient::getNumericAssignmentDetails(
    const std::string& flagKey, const std::string& subjectKey, const Attributes& subjectAttributes,
    double defaultValue) {
    auto config = configurationStore_.getConfiguration();
    return evaluationClient(*config).getNumericAssignmentDetails(flagKey, subjectKey,
                                                                 subjectAttributes, defaultValue);
}

EvaluationResult<std::string> EppoClient::getStringAssignmentDetails(
    const std::string& flagKey, const std::string& subjectKey, const Attributes& subjectAttributes,
    const std::string& defaultValue) {
    auto config = configurationStore_.getConfiguration();
    return evaluationClient(*config).getStringAssignmentDetails(flagKey, subjectKey,
                                                                subjectAttributes, defaultValue);
}

EvaluationResult<nlohmann::json> EppoClient::getJsonAssignmentDetails(
    const std::string& flagKey, const std::string& subjectKey, const Attributes& subjectAttributes,
    const nlohmann::json& defaultValue) {
    auto config = configurationStore_.getConfiguration();
    return evaluationClient(*config).getJsonAssignmentDetails(flagKey, subjectKey,
                                                              subjectAttributes, defaultValue);
}

EvaluationResult<std::string> EppoClient::getSerializedJsonAssignmentDetails(
    const std::string& flagKey, const std::string& subjectKey, const Attributes& subjectAttributes,
    const std::string& defaultValue) {
    auto config = configurationStore_.getConfiguration();
    return evaluationClient(*config).getSerializedJsonAssignmentDetails(
        flagKey, subjectKey, subjectAttributes, defaultValue);
}


}  // namespace eppoclient

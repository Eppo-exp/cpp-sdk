#include "evaluation_client.hpp"
#include <chrono>
#include "time_utils.hpp"

namespace eppoclient {

// ============================================================================
// EvaluationClient Implementation
// ============================================================================

EvaluationClient::EvaluationClient(const Configuration& configuration,
                                   AssignmentLogger& assignmentLogger, BanditLogger& banditLogger,
                                   ApplicationLogger& applicationLogger)
    : configuration_(configuration),
      assignmentLogger_(assignmentLogger),
      banditLogger_(banditLogger),
      applicationLogger_(applicationLogger) {}

bool EvaluationClient::getBoolAssignment(const std::string& flagKey, const std::string& subjectKey,
                                         const Attributes& subjectAttributes, bool defaultValue) {
    auto variation = getAssignment(configuration_, flagKey, subjectKey, subjectAttributes,
                                   VariationType::BOOLEAN);
    return extractVariation(variation, flagKey, VariationType::BOOLEAN, defaultValue);
}

double EvaluationClient::getNumericAssignment(const std::string& flagKey,
                                              const std::string& subjectKey,
                                              const Attributes& subjectAttributes,
                                              double defaultValue) {
    auto variation = getAssignment(configuration_, flagKey, subjectKey, subjectAttributes,
                                   VariationType::NUMERIC);
    return extractVariation(variation, flagKey, VariationType::NUMERIC, defaultValue);
}

int64_t EvaluationClient::getIntegerAssignment(const std::string& flagKey,
                                               const std::string& subjectKey,
                                               const Attributes& subjectAttributes,
                                               int64_t defaultValue) {
    auto variation = getAssignment(configuration_, flagKey, subjectKey, subjectAttributes,
                                   VariationType::INTEGER);
    return extractVariation(variation, flagKey, VariationType::INTEGER, defaultValue);
}

std::string EvaluationClient::getStringAssignment(const std::string& flagKey,
                                                  const std::string& subjectKey,
                                                  const Attributes& subjectAttributes,
                                                  const std::string& defaultValue) {
    auto variation = getAssignment(configuration_, flagKey, subjectKey, subjectAttributes,
                                   VariationType::STRING);
    return extractVariation(variation, flagKey, VariationType::STRING, defaultValue);
}

nlohmann::json EvaluationClient::getJSONAssignment(const std::string& flagKey,
                                                   const std::string& subjectKey,
                                                   const Attributes& subjectAttributes,
                                                   const nlohmann::json& defaultValue) {
    auto variation =
        getAssignment(configuration_, flagKey, subjectKey, subjectAttributes, VariationType::JSON);
    return extractVariation(variation, flagKey, VariationType::JSON, defaultValue);
}

std::string EvaluationClient::getSerializedJSONAssignment(const std::string& flagKey,
                                                          const std::string& subjectKey,
                                                          const Attributes& subjectAttributes,
                                                          const std::string& defaultValue) {
    auto variation =
        getAssignment(configuration_, flagKey, subjectKey, subjectAttributes, VariationType::JSON);

    if (!variation.has_value()) {
        return defaultValue;
    }

    if (!std::holds_alternative<nlohmann::json>(*variation)) {
        std::string actualType = detectVariationType(*variation);
        std::string expectedType = variationTypeToString(VariationType::JSON);
        applicationLogger_.error("Variation value does not have the correct type. Found " +
                                 actualType + ", but expected " + expectedType + " for flag " +
                                 flagKey);
        return defaultValue;
    }

    return std::get<nlohmann::json>(*variation).dump();
}

std::optional<std::variant<std::string, int64_t, double, bool, nlohmann::json>>
EvaluationClient::getAssignment(const Configuration& config, const std::string& flagKey,
                                const std::string& subjectKey, const Attributes& subjectAttributes,
                                VariationType variationType) {
    // Validate inputs
    if (subjectKey.empty()) {
        applicationLogger_.error("No subject key provided");
        return std::nullopt;
    }

    if (flagKey.empty()) {
        applicationLogger_.error("No flag key provided");
        return std::nullopt;
    }

    // Get flag configuration
    const FlagConfiguration* flag = config.getFlagConfiguration(flagKey);
    if (flag == nullptr) {
        applicationLogger_.info("Failed to get flag configuration for: " + flagKey);
        return std::nullopt;
    }

    // Verify flag type
    if (!verifyType(*flag, variationType)) {
        applicationLogger_.warn("Failed to verify flag type for: " + flagKey +
                                " (expected: " + variationTypeToString(variationType) +
                                ", actual: " + variationTypeToString(flag->variationType) + ")");
        return std::nullopt;
    }

    // Evaluate flag
    std::optional<EvalResult> result =
        evalFlag(*flag, subjectKey, subjectAttributes, &applicationLogger_);
    if (!result.has_value()) {
        applicationLogger_.info("Failed to evaluate flag: " + flagKey);
        return std::nullopt;
    }

    // Log assignment event
    logAssignment(result->event);

    return result->value;
}

void EvaluationClient::logAssignment(const std::optional<AssignmentEvent>& event) {
    if (!event.has_value()) {
        return;
    }

    assignmentLogger_.logAssignment(*event);
}

void EvaluationClient::logBanditAction(const BanditEvent& event) {
    banditLogger_.logBanditAction(event);
}

BanditResult EvaluationClient::getBanditAction(
    const std::string& flagKey, const std::string& subjectKey,
    const ContextAttributes& subjectAttributes,
    const std::map<std::string, ContextAttributes>& actions, const std::string& defaultVariation) {
    // Ignoring the error here as we can always proceed with default variation
    std::string variation = defaultVariation;
    auto assignmentValue =
        getAssignment(configuration_, flagKey, subjectKey, toGenericAttributes(subjectAttributes),
                      VariationType::STRING);
    if (assignmentValue.has_value() && std::holds_alternative<std::string>(*assignmentValue)) {
        variation = std::get<std::string>(*assignmentValue);
    }

    // If no actions have been passed, return the variation with no action
    if (actions.empty()) {
        return BanditResult(variation, std::nullopt);
    }

    // Get bandit variation
    BanditVariation banditVariation;
    if (!configuration_.getBanditVariant(flagKey, variation, banditVariation)) {
        return BanditResult(variation, std::nullopt);
    }

    // Get bandit configuration
    const BanditConfiguration* bandit = configuration_.getBanditConfiguration(banditVariation.key);
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
    BanditEvent event =
        createBanditEvent(flagKey, subjectKey, bandit->banditKey, bandit->modelVersion, evaluation,
                          formatISOTimestamp(std::chrono::system_clock::now()));

    logBanditAction(event);

    return BanditResult(variation, evaluation.actionKey);
}

EvaluationResult<std::string> EvaluationClient::getBanditActionDetails(
    const std::string& flagKey, const std::string& subjectKey,
    const ContextAttributes& subjectAttributes,
    const std::map<std::string, ContextAttributes>& actions, const std::string& defaultVariation) {
    auto assignmentResult = getStringAssignmentDetails(
        flagKey, subjectKey, toGenericAttributes(subjectAttributes), defaultVariation);

    std::string variation = assignmentResult.variation;
    EvaluationDetails details = assignmentResult.evaluationDetails.value_or(EvaluationDetails());

    // If no actions have been passed, return the variation with details
    if (actions.empty()) {
        details.banditEvaluationCode = BanditEvaluationCode::NO_ACTIONS_SUPPLIED_FOR_BANDIT;
        return EvaluationResult<std::string>(variation, std::nullopt, details);
    }

    // Get bandit variation
    BanditVariation banditVariation;
    if (!configuration_.getBanditVariant(flagKey, variation, banditVariation)) {
        details.banditEvaluationCode = BanditEvaluationCode::NON_BANDIT_VARIATION;
        return EvaluationResult<std::string>(variation, std::nullopt, details);
    }

    // Get bandit configuration
    const BanditConfiguration* bandit = configuration_.getBanditConfiguration(banditVariation.key);
    if (bandit == nullptr) {
        details.banditEvaluationCode = BanditEvaluationCode::CONFIGURATION_MISSING;
        return EvaluationResult<std::string>(variation, std::nullopt, details);
    }

    // Evaluate bandit
    BanditEvaluationContext evalContext;
    evalContext.flagKey = flagKey;
    evalContext.subjectKey = subjectKey;
    evalContext.subjectAttributes = subjectAttributes;
    evalContext.actions = actions;

    BanditEvaluationDetails evaluation = evaluateBandit(bandit->modelData, evalContext);

    // Log bandit action
    BanditEvent event = createBanditEvent(flagKey, subjectKey, bandit->banditKey,
                                          bandit->modelVersion, evaluation, details.timestamp);

    logBanditAction(event);

    // Set bandit details
    details.banditEvaluationCode = BanditEvaluationCode::MATCH;
    details.banditKey = bandit->banditKey;
    details.banditAction = evaluation.actionKey;

    return EvaluationResult<std::string>(variation, evaluation.actionKey, details);
}

// ============================================================================
// Assignment Details Methods
// ============================================================================

EvaluationResult<bool> EvaluationClient::getBooleanAssignmentDetails(
    const std::string& flagKey, const std::string& subjectKey, const Attributes& subjectAttributes,
    bool defaultValue) {
    return getAssignmentDetails<bool>(VariationType::BOOLEAN, flagKey, subjectKey,
                                      subjectAttributes, defaultValue);
}

EvaluationResult<int64_t> EvaluationClient::getIntegerAssignmentDetails(
    const std::string& flagKey, const std::string& subjectKey, const Attributes& subjectAttributes,
    int64_t defaultValue) {
    return getAssignmentDetails<int64_t>(VariationType::INTEGER, flagKey, subjectKey,
                                         subjectAttributes, defaultValue);
}

EvaluationResult<double> EvaluationClient::getNumericAssignmentDetails(
    const std::string& flagKey, const std::string& subjectKey, const Attributes& subjectAttributes,
    double defaultValue) {
    return getAssignmentDetails<double>(VariationType::NUMERIC, flagKey, subjectKey,
                                        subjectAttributes, defaultValue);
}

EvaluationResult<std::string> EvaluationClient::getStringAssignmentDetails(
    const std::string& flagKey, const std::string& subjectKey, const Attributes& subjectAttributes,
    const std::string& defaultValue) {
    return getAssignmentDetails<std::string>(VariationType::STRING, flagKey, subjectKey,
                                             subjectAttributes, defaultValue);
}

EvaluationResult<nlohmann::json> EvaluationClient::getJsonAssignmentDetails(
    const std::string& flagKey, const std::string& subjectKey, const Attributes& subjectAttributes,
    const nlohmann::json& defaultValue) {
    return getAssignmentDetails<nlohmann::json>(VariationType::JSON, flagKey, subjectKey,
                                                subjectAttributes, defaultValue);
}

EvaluationResult<std::string> EvaluationClient::getSerializedJsonAssignmentDetails(
    const std::string& flagKey, const std::string& subjectKey, const Attributes& subjectAttributes,
    const std::string& defaultValue) {
    // Get JSON assignment details first
    nlohmann::json defaultJson = nlohmann::json::parse(defaultValue.empty() ? "{}" : defaultValue);
    auto jsonResult = getJsonAssignmentDetails(flagKey, subjectKey, subjectAttributes, defaultJson);

    // Convert JSON variation to string
    std::string stringifiedVariation = jsonResult.variation.dump();

    // Return with stringified value but same details
    return EvaluationResult<std::string>(stringifiedVariation, jsonResult.action,
                                         jsonResult.evaluationDetails);
}


}  // namespace eppoclient

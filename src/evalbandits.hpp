#ifndef EVALBANDITS_HPP
#define EVALBANDITS_HPP

#include <map>
#include <optional>
#include <string>
#include "bandit_model.hpp"
#include "rules.hpp"

namespace eppoclient {

/**
 * Context attributes for bandit evaluation.
 * Separates attributes into numeric and categorical for model scoring.
 */
struct ContextAttributes {
    std::map<std::string, double> numericAttributes;
    std::map<std::string, std::string> categoricalAttributes;

    ContextAttributes() = default;
};

/**
 * Result of a bandit action evaluation.
 */
struct BanditResult {
    std::string variation;
    std::optional<std::string> action;

    BanditResult() = default;
    BanditResult(const std::string& var, const std::optional<std::string>& act)
        : variation(var), action(act) {}
};

/**
 * Event data for bandit action logging.
 */
struct BanditEvent {
    std::string flagKey;
    std::string banditKey;
    std::string subject;
    std::string action;
    double actionProbability;
    double optimalityGap;
    std::string modelVersion;
    std::string timestamp;
    std::map<std::string, double> subjectNumericAttributes;
    std::map<std::string, std::string> subjectCategoricalAttributes;
    std::map<std::string, double> actionNumericAttributes;
    std::map<std::string, std::string> actionCategoricalAttributes;
    std::map<std::string, std::string> metaData;

    BanditEvent() : actionProbability(0.0), optimalityGap(0.0) {}
};

/**
 * Infers ContextAttributes from generic Attributes.
 * Converts numeric types (int64_t, double) to numeric attributes
 * and string/bool types to categorical attributes.
 */
ContextAttributes inferContextAttributes(const Attributes& attrs);

/**
 * Converts ContextAttributes to generic Attributes.
 */
Attributes toGenericAttributes(const ContextAttributes& contextAttrs);

/**
 * Internal context for bandit evaluation.
 */
struct BanditEvaluationContext {
    std::string flagKey;
    std::string subjectKey;
    ContextAttributes subjectAttributes;
    std::map<std::string, ContextAttributes> actions;
};

/**
 * Detailed result of bandit evaluation.
 */
struct BanditEvaluationDetails {
    std::string flagKey;
    std::string subjectKey;
    ContextAttributes subjectAttributes;
    std::string actionKey;
    ContextAttributes actionAttributes;
    double actionScore;
    double actionWeight;
    double gamma;
    double optimalityGap;

    BanditEvaluationDetails()
        : actionScore(0.0), actionWeight(0.0), gamma(0.0), optimalityGap(0.0) {}
};

/**
 * Evaluates a bandit model to select the best action.
 * Implements the contextual bandit algorithm with Thompson sampling.
 */
BanditEvaluationDetails evaluateBandit(const BanditModelData& modelData,
                                       const BanditEvaluationContext& context);

/**
 * Scores a single action using the bandit model coefficients.
 */
double scoreAction(const BanditModelData& modelData, const ContextAttributes& subjectAttributes,
                   const std::string& actionKey, const ContextAttributes& actionAttributes);

/**
 * Scores numeric attributes using coefficients.
 */
double scoreNumericAttributes(const std::vector<BanditNumericAttributeCoefficient>& coefficients,
                              const std::map<std::string, double>& attributes);

/**
 * Scores categorical attributes using coefficients.
 */
double scoreCategoricalAttributes(
    const std::vector<BanditCategoricalAttributeCoefficient>& coefficients,
    const std::map<std::string, std::string>& attributes);

/**
 * Computes shard value for deterministic randomization.
 */
int64_t getShard(const std::string& input, int64_t totalShards);

/**
 * Creates a BanditEvent from evaluation results.
 * Centralizes the logic for constructing bandit logging events.
 */
BanditEvent createBanditEvent(const std::string& flagKey, const std::string& subjectKey,
                              const std::string& banditKey, const std::string& modelVersion,
                              const BanditEvaluationDetails& evaluation,
                              const std::string& timestamp);

}  // namespace eppoclient

#endif  // EVALBANDITS_H

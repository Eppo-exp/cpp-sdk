#include "evalbandits.hpp"
#include "evalflags.hpp"
#include <algorithm>
#include <limits>
#include <vector>

namespace eppoclient {

ContextAttributes inferContextAttributes(const Attributes& attrs) {
    ContextAttributes result;

    for (const auto& [key, value] : attrs) {
        if (std::holds_alternative<int64_t>(value)) {
            result.numericAttributes[key] = static_cast<double>(std::get<int64_t>(value));
        } else if (std::holds_alternative<double>(value)) {
            result.numericAttributes[key] = std::get<double>(value);
        } else if (std::holds_alternative<std::string>(value)) {
            result.categoricalAttributes[key] = std::get<std::string>(value);
        } else if (std::holds_alternative<bool>(value)) {
            result.categoricalAttributes[key] = std::get<bool>(value) ? "true" : "false";
        }
        // std::monostate (null) is silently dropped
    }

    return result;
}

Attributes toGenericAttributes(const ContextAttributes& contextAttrs) {
    Attributes result;

    for (const auto& [key, value] : contextAttrs.numericAttributes) {
        result[key] = value;
    }

    for (const auto& [key, value] : contextAttrs.categoricalAttributes) {
        result[key] = value;
    }

    return result;
}

double scoreNumericAttributes(
    const std::vector<BanditNumericAttributeCoefficient>& coefficients,
    const std::map<std::string, double>& attributes) {

    double score = 0.0;
    for (const auto& coefficient : coefficients) {
        auto it = attributes.find(coefficient.attributeKey);
        if (it != attributes.end()) {
            score += coefficient.coefficient * it->second;
        } else {
            score += coefficient.missingValueCoefficient;
        }
    }
    return score;
}

double scoreCategoricalAttributes(
    const std::vector<BanditCategoricalAttributeCoefficient>& coefficients,
    const std::map<std::string, std::string>& attributes) {

    double score = 0.0;
    for (const auto& coefficient : coefficients) {
        auto attrIt = attributes.find(coefficient.attributeKey);
        if (attrIt != attributes.end()) {
            auto valueIt = coefficient.valueCoefficients.find(attrIt->second);
            if (valueIt != coefficient.valueCoefficients.end()) {
                score += valueIt->second;
            } else {
                score += coefficient.missingValueCoefficient;
            }
        } else {
            score += coefficient.missingValueCoefficient;
        }
    }
    return score;
}

double scoreAction(const BanditModelData& modelData,
                  const ContextAttributes& subjectAttributes,
                  const std::string& actionKey,
                  const ContextAttributes& actionAttributes) {

    auto coeffIt = modelData.coefficients.find(actionKey);
    if (coeffIt == modelData.coefficients.end()) {
        return modelData.defaultActionScore;
    }

    const BanditCoefficients& coefficients = coeffIt->second;

    double score = coefficients.intercept;
    score += scoreNumericAttributes(coefficients.actionNumericCoefficients,
                                     actionAttributes.numericAttributes);
    score += scoreCategoricalAttributes(coefficients.actionCategoricalCoefficients,
                                        actionAttributes.categoricalAttributes);
    score += scoreNumericAttributes(coefficients.subjectNumericCoefficients,
                                     subjectAttributes.numericAttributes);
    score += scoreCategoricalAttributes(coefficients.subjectCategoricalCoefficients,
                                        subjectAttributes.categoricalAttributes);

    return score;
}

BanditEvaluationDetails evaluateBandit(const BanditModelData& modelData,
                                       const BanditEvaluationContext& context) {
    const int64_t totalShards = 10000;
    const size_t nActions = context.actions.size();

    // Score all actions
    std::map<std::string, double> scores;
    for (const auto& [actionKey, actionAttributes] : context.actions) {
        scores[actionKey] = scoreAction(modelData, context.subjectAttributes,
                                       actionKey, actionAttributes);
    }

    // Find best action and best score
    std::string bestAction;
    double bestScore = -std::numeric_limits<double>::infinity();
    for (const auto& [actionKey, score] : scores) {
        if (score > bestScore || (score == bestScore && actionKey < bestAction)) {
            bestAction = actionKey;
            bestScore = score;
        }
    }

    // Calculate weights for each action
    std::map<std::string, double> weights;
    for (const auto& [actionKey, score] : scores) {
        if (actionKey == bestAction) {
            // Best action weight is calculated as remainder
            continue;
        }

        // Adjust probability floor for number of actions to control the sum
        double minProbability = modelData.actionProbabilityFloor / static_cast<double>(nActions);
        double weight = 1.0 / (static_cast<double>(nActions) + modelData.gamma * (bestScore - score));
        weights[actionKey] = std::max(minProbability, weight);
    }

    // Calculate remainder weight for best action
    double remainderWeight = 1.0;
    for (const auto& [_, weight] : weights) {
        remainderWeight -= weight;
    }
    weights[bestAction] = std::max(0.0, remainderWeight);

    // Pseudo-random deterministic shuffle of actions
    std::vector<std::string> shuffledActions;
    std::map<std::string, int64_t> shards;

    for (const auto& [actionKey, _] : context.actions) {
        shuffledActions.push_back(actionKey);
        shards[actionKey] = getShard(context.flagKey + "-" + context.subjectKey + "-" + actionKey,
                                     totalShards);
    }

    // Sort actions by their shard value, using action key as tie breaker
    std::sort(shuffledActions.begin(), shuffledActions.end(),
              [&shards](const std::string& a1, const std::string& a2) {
                  int64_t v1 = shards[a1];
                  int64_t v2 = shards[a2];
                  if (v1 < v2) {
                      return true;
                  } else if (v1 > v2) {
                      return false;
                  } else {
                      // Tie-breaking
                      return a1 < a2;
                  }
              });

    // Select action based on shard value
    double shardValue = static_cast<double>(getShard(context.flagKey + "-" + context.subjectKey,
                                                      totalShards)) / static_cast<double>(totalShards);

    double cumulativeWeight = 0.0;
    std::string selectedAction = shuffledActions.back();  // Default to last action
    for (const auto& actionKey : shuffledActions) {
        cumulativeWeight += weights[actionKey];
        if (cumulativeWeight > shardValue) {
            selectedAction = actionKey;
            break;
        }
    }

    double optimalityGap = bestScore - scores[selectedAction];

    BanditEvaluationDetails details;
    details.flagKey = context.flagKey;
    details.subjectKey = context.subjectKey;
    details.subjectAttributes = context.subjectAttributes;
    details.actionKey = selectedAction;
    details.actionAttributes = context.actions.at(selectedAction);
    details.actionScore = scores[selectedAction];
    details.actionWeight = weights[selectedAction];
    details.gamma = modelData.gamma;
    details.optimalityGap = optimalityGap;

    return details;
}

BanditEvent createBanditEvent(const std::string& flagKey,
                             const std::string& subjectKey,
                             const std::string& banditKey,
                             const std::string& modelVersion,
                             const BanditEvaluationDetails& evaluation,
                             const std::string& timestamp) {
    BanditEvent event;
    event.flagKey = flagKey;
    event.banditKey = banditKey;
    event.subject = subjectKey;
    event.action = evaluation.actionKey;
    event.actionProbability = evaluation.actionWeight;
    event.optimalityGap = evaluation.optimalityGap;
    event.modelVersion = modelVersion;
    event.timestamp = timestamp;
    event.subjectNumericAttributes = evaluation.subjectAttributes.numericAttributes;
    event.subjectCategoricalAttributes = evaluation.subjectAttributes.categoricalAttributes;
    event.actionNumericAttributes = evaluation.actionAttributes.numericAttributes;
    event.actionCategoricalAttributes = evaluation.actionAttributes.categoricalAttributes;
    event.metaData["sdkLanguage"] = "cpp";
    event.metaData["sdkVersion"] = SDK_VERSION;
    return event;
}

} // namespace eppoclient

#ifndef BANDIT_MODEL_HPP
#define BANDIT_MODEL_HPP

#include <chrono>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace eppoclient {

/**
 * Coefficient for a numeric attribute in the bandit model.
 * Represents linear coefficients for continuous/numeric features.
 */
struct BanditNumericAttributeCoefficient {
    std::string attributeKey;
    double coefficient;
    double missingValueCoefficient;

    BanditNumericAttributeCoefficient() : coefficient(0.0), missingValueCoefficient(0.0) {}
};

void to_json(nlohmann::json& j, const BanditNumericAttributeCoefficient& bnac);

/**
 * Coefficient for a categorical attribute in the bandit model.
 * Represents one-hot encoded coefficients for categorical features.
 */
struct BanditCategoricalAttributeCoefficient {
    std::string attributeKey;
    double missingValueCoefficient;
    std::map<std::string, double> valueCoefficients;

    BanditCategoricalAttributeCoefficient() : missingValueCoefficient(0.0) {}
};

void to_json(nlohmann::json& j, const BanditCategoricalAttributeCoefficient& bcac);

/**
 * Coefficients for a single action in the bandit model.
 * Contains both numeric and categorical coefficients for subject and action attributes.
 */
struct BanditCoefficients {
    std::string actionKey;
    double intercept;
    std::vector<BanditNumericAttributeCoefficient> subjectNumericCoefficients;
    std::vector<BanditCategoricalAttributeCoefficient> subjectCategoricalCoefficients;
    std::vector<BanditNumericAttributeCoefficient> actionNumericCoefficients;
    std::vector<BanditCategoricalAttributeCoefficient> actionCategoricalCoefficients;

    BanditCoefficients() : intercept(0.0) {}
};

void to_json(nlohmann::json& j, const BanditCoefficients& bc);

/**
 * Model data for the bandit algorithm.
 * Contains gamma (exploration parameter), default scores, and coefficients for all actions.
 */
struct BanditModelData {
    double gamma;
    double defaultActionScore;
    double actionProbabilityFloor;
    std::map<std::string, BanditCoefficients> coefficients;

    BanditModelData() : gamma(0.0), defaultActionScore(0.0), actionProbabilityFloor(0.0) {}
};

void to_json(nlohmann::json& j, const BanditModelData& bmd);

/**
 * Configuration for a single bandit.
 * Contains the model metadata and the actual model data.
 */
struct BanditConfiguration {
    std::string banditKey;
    std::string modelName;
    std::string modelVersion;
    BanditModelData modelData;
    std::chrono::system_clock::time_point updatedAt;

    BanditConfiguration() = default;
};

void to_json(nlohmann::json& j, const BanditConfiguration& bc);

/**
 * Response containing all bandit configurations.
 * This is the top-level structure returned from the bandit configuration endpoint.
 */
struct BanditResponse {
    std::map<std::string, BanditConfiguration> bandits;
    std::chrono::system_clock::time_point updatedAt;

    BanditResponse() = default;
};

void to_json(nlohmann::json& j, const BanditResponse& br);
void from_json(const nlohmann::json& j, BanditResponse& br);

/**
 * Associates a bandit with a specific flag variation.
 * Used to link feature flag variations to bandit experiments.
 */
struct BanditVariation {
    std::string key;
    std::string flagKey;
    std::string variationKey;
    std::string variationValue;

    BanditVariation() = default;
};

void to_json(nlohmann::json& j, const BanditVariation& bv);

// Internal namespace for implementation details not covered by semver
namespace internal {

// Custom parsing functions that handle errors gracefully.
// These are INTERNAL APIs and may change without notice.
std::optional<BanditNumericAttributeCoefficient> parseBanditNumericAttributeCoefficient(
    const nlohmann::json& j, std::string& error);
std::optional<BanditCategoricalAttributeCoefficient> parseBanditCategoricalAttributeCoefficient(
    const nlohmann::json& j, std::string& error);
std::optional<BanditCoefficients> parseBanditCoefficients(const nlohmann::json& j,
                                                          std::string& error);
std::optional<BanditModelData> parseBanditModelData(const nlohmann::json& j, std::string& error);
std::optional<BanditConfiguration> parseBanditConfiguration(const nlohmann::json& j,
                                                            std::string& error);
std::optional<BanditVariation> parseBanditVariation(const nlohmann::json& j, std::string& error);

}  // namespace internal

}  // namespace eppoclient

#endif  // BANDIT_MODEL_H

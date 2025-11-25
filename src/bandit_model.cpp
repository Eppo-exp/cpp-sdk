#include "bandit_model.hpp"
#include <chrono>
#include "time_utils.hpp"
namespace eppoclient {

// BanditNumericAttributeCoefficient serialization
void to_json(nlohmann::json& j, const BanditNumericAttributeCoefficient& bnac) {
    j = nlohmann::json{{"attributeKey", bnac.attributeKey},
                       {"coefficient", bnac.coefficient},
                       {"missingValueCoefficient", bnac.missingValueCoefficient}};
}

void from_json(const nlohmann::json& j, BanditNumericAttributeCoefficient& bnac) {
    j.at("attributeKey").get_to(bnac.attributeKey);
    j.at("coefficient").get_to(bnac.coefficient);
    j.at("missingValueCoefficient").get_to(bnac.missingValueCoefficient);
}

// BanditCategoricalAttributeCoefficient serialization
void to_json(nlohmann::json& j, const BanditCategoricalAttributeCoefficient& bcac) {
    j = nlohmann::json{{"attributeKey", bcac.attributeKey},
                       {"missingValueCoefficient", bcac.missingValueCoefficient},
                       {"valueCoefficients", bcac.valueCoefficients}};
}

void from_json(const nlohmann::json& j, BanditCategoricalAttributeCoefficient& bcac) {
    j.at("attributeKey").get_to(bcac.attributeKey);
    j.at("missingValueCoefficient").get_to(bcac.missingValueCoefficient);
    j.at("valueCoefficients").get_to(bcac.valueCoefficients);
}

// BanditCoefficients serialization
void to_json(nlohmann::json& j, const BanditCoefficients& bc) {
    j = nlohmann::json{{"actionKey", bc.actionKey},
                       {"intercept", bc.intercept},
                       {"subjectNumericCoefficients", bc.subjectNumericCoefficients},
                       {"subjectCategoricalCoefficients", bc.subjectCategoricalCoefficients},
                       {"actionNumericCoefficients", bc.actionNumericCoefficients},
                       {"actionCategoricalCoefficients", bc.actionCategoricalCoefficients}};
}

void from_json(const nlohmann::json& j, BanditCoefficients& bc) {
    j.at("actionKey").get_to(bc.actionKey);
    j.at("intercept").get_to(bc.intercept);
    j.at("subjectNumericCoefficients").get_to(bc.subjectNumericCoefficients);
    j.at("subjectCategoricalCoefficients").get_to(bc.subjectCategoricalCoefficients);
    j.at("actionNumericCoefficients").get_to(bc.actionNumericCoefficients);
    j.at("actionCategoricalCoefficients").get_to(bc.actionCategoricalCoefficients);
}

// BanditModelData serialization
void to_json(nlohmann::json& j, const BanditModelData& bmd) {
    j = nlohmann::json{{"gamma", bmd.gamma},
                       {"defaultActionScore", bmd.defaultActionScore},
                       {"actionProbabilityFloor", bmd.actionProbabilityFloor},
                       {"coefficients", bmd.coefficients}};
}

void from_json(const nlohmann::json& j, BanditModelData& bmd) {
    j.at("gamma").get_to(bmd.gamma);
    j.at("defaultActionScore").get_to(bmd.defaultActionScore);
    j.at("actionProbabilityFloor").get_to(bmd.actionProbabilityFloor);
    j.at("coefficients").get_to(bmd.coefficients);
}

// BanditConfiguration serialization
void to_json(nlohmann::json& j, const BanditConfiguration& bc) {
    j = nlohmann::json{{"banditKey", bc.banditKey},
                       {"modelName", bc.modelName},
                       {"modelVersion", bc.modelVersion},
                       {"modelData", bc.modelData},
                       {"updatedAt", formatISOTimestamp(bc.updatedAt)}};
}

void from_json(const nlohmann::json& j, BanditConfiguration& bc) {
    j.at("banditKey").get_to(bc.banditKey);
    j.at("modelName").get_to(bc.modelName);
    j.at("modelVersion").get_to(bc.modelVersion);
    j.at("modelData").get_to(bc.modelData);

    if (j.contains("updatedAt") && j["updatedAt"].is_string()) {
        bc.updatedAt = parseISOTimestamp(j["updatedAt"].get<std::string>());
    }
}

// BanditResponse serialization
void to_json(nlohmann::json& j, const BanditResponse& br) {
    j = nlohmann::json{{"bandits", br.bandits}, {"updatedAt", formatISOTimestamp(br.updatedAt)}};
}

void from_json(const nlohmann::json& j, BanditResponse& br) {
    j.at("bandits").get_to(br.bandits);

    if (j.contains("updatedAt") && j["updatedAt"].is_string()) {
        br.updatedAt = parseISOTimestamp(j["updatedAt"].get<std::string>());
    }
}

// BanditVariation serialization
void to_json(nlohmann::json& j, const BanditVariation& bv) {
    j = nlohmann::json{{"key", bv.key},
                       {"flagKey", bv.flagKey},
                       {"variationKey", bv.variationKey},
                       {"variationValue", bv.variationValue}};
}

void from_json(const nlohmann::json& j, BanditVariation& bv) {
    j.at("key").get_to(bv.key);
    j.at("flagKey").get_to(bv.flagKey);
    j.at("variationKey").get_to(bv.variationKey);
    j.at("variationValue").get_to(bv.variationValue);
}

}  // namespace eppoclient

#include "bandit_model.hpp"
#include <chrono>
#include "json_utils.hpp"
#include "time_utils.hpp"

namespace eppoclient {

// Internal namespace for implementation details not covered by semver
namespace internal {

// Custom parsing functions that handle errors gracefully
std::optional<BanditNumericAttributeCoefficient> parseBanditNumericAttributeCoefficient(
    const nlohmann::json& j, std::string& error) {
    TRY_GET_REQUIRED(attributeKey, std::string, j, "attributeKey",
                     "BanditNumericAttributeCoefficient", error);
    TRY_GET_REQUIRED(coefficient, double, j, "coefficient", "BanditNumericAttributeCoefficient",
                     error);
    TRY_GET_REQUIRED(missingValueCoefficient, double, j, "missingValueCoefficient",
                     "BanditNumericAttributeCoefficient", error);

    BanditNumericAttributeCoefficient bnac;
    bnac.attributeKey = *attributeKey;
    bnac.coefficient = *coefficient;
    bnac.missingValueCoefficient = *missingValueCoefficient;
    return bnac;
}

std::optional<BanditCategoricalAttributeCoefficient> parseBanditCategoricalAttributeCoefficient(
    const nlohmann::json& j, std::string& error) {
    TRY_GET_REQUIRED(attributeKey, std::string, j, "attributeKey",
                     "BanditCategoricalAttributeCoefficient", error);
    TRY_GET_REQUIRED(missingValueCoefficient, double, j, "missingValueCoefficient",
                     "BanditCategoricalAttributeCoefficient", error);

    // valueCoefficients is an object map - check manually since we can't use getRequiredField for
    // maps
    if (!j.contains("valueCoefficients")) {
        error = "BanditCategoricalAttributeCoefficient: Missing required field: valueCoefficients";
        return std::nullopt;
    }
    if (!j["valueCoefficients"].is_object()) {
        error =
            "BanditCategoricalAttributeCoefficient: Field 'valueCoefficients' must be an object";
        return std::nullopt;
    }

    BanditCategoricalAttributeCoefficient bcac;
    bcac.attributeKey = *attributeKey;
    bcac.missingValueCoefficient = *missingValueCoefficient;

    // Safely parse the map manually
    for (auto& [key, value] : j["valueCoefficients"].items()) {
        if (!value.is_number()) {
            error =
                "BanditCategoricalAttributeCoefficient: valueCoefficients values must be numbers";
            return std::nullopt;
        }
        bcac.valueCoefficients[key] =
            value.is_number_float()
                ? value.get_ref<const nlohmann::json::number_float_t&>()
                : static_cast<double>(value.get_ref<const nlohmann::json::number_integer_t&>());
    }

    return bcac;
}

std::optional<BanditCoefficients> parseBanditCoefficients(const nlohmann::json& j,
                                                          std::string& error) {
    TRY_GET_REQUIRED(actionKey, std::string, j, "actionKey", "BanditCoefficients", error);
    TRY_GET_REQUIRED(intercept, double, j, "intercept", "BanditCoefficients", error);

    // Check required array fields
    auto checkArrayField = [&](std::string_view fieldName) -> bool {
        if (!j.contains(fieldName)) {
            error = std::string("BanditCoefficients: Missing required field: ") +
                    std::string(fieldName);
            return false;
        }
        if (!j[fieldName].is_array()) {
            error = std::string("BanditCoefficients: Field '") + std::string(fieldName) +
                    "' must be an array";
            return false;
        }
        return true;
    };

    if (!checkArrayField("subjectNumericCoefficients"))
        return std::nullopt;
    if (!checkArrayField("subjectCategoricalCoefficients"))
        return std::nullopt;
    if (!checkArrayField("actionNumericCoefficients"))
        return std::nullopt;
    if (!checkArrayField("actionCategoricalCoefficients"))
        return std::nullopt;

    BanditCoefficients bc;
    bc.actionKey = *actionKey;
    bc.intercept = *intercept;

    // Parse numeric coefficients arrays
    for (const auto& coeffJson : j["subjectNumericCoefficients"]) {
        TRY_PARSE(coeff, parseBanditNumericAttributeCoefficient, coeffJson,
                  "BanditCoefficients: Invalid subjectNumericCoefficient: ", error);
        bc.subjectNumericCoefficients.push_back(*coeff);
    }

    // Parse categorical coefficients arrays
    for (const auto& coeffJson : j["subjectCategoricalCoefficients"]) {
        TRY_PARSE(coeff, parseBanditCategoricalAttributeCoefficient, coeffJson,
                  "BanditCoefficients: Invalid subjectCategoricalCoefficient: ", error);
        bc.subjectCategoricalCoefficients.push_back(*coeff);
    }

    for (const auto& coeffJson : j["actionNumericCoefficients"]) {
        TRY_PARSE(coeff, parseBanditNumericAttributeCoefficient, coeffJson,
                  "BanditCoefficients: Invalid actionNumericCoefficient: ", error);
        bc.actionNumericCoefficients.push_back(*coeff);
    }

    for (const auto& coeffJson : j["actionCategoricalCoefficients"]) {
        TRY_PARSE(coeff, parseBanditCategoricalAttributeCoefficient, coeffJson,
                  "BanditCoefficients: Invalid actionCategoricalCoefficient: ", error);
        bc.actionCategoricalCoefficients.push_back(*coeff);
    }

    return bc;
}

std::optional<BanditModelData> parseBanditModelData(const nlohmann::json& j, std::string& error) {
    TRY_GET_REQUIRED(gamma, double, j, "gamma", "BanditModelData", error);
    TRY_GET_REQUIRED(defaultActionScore, double, j, "defaultActionScore", "BanditModelData", error);
    TRY_GET_REQUIRED(actionProbabilityFloor, double, j, "actionProbabilityFloor", "BanditModelData",
                     error);

    // coefficients is an object map - check manually
    if (!j.contains("coefficients")) {
        error = "BanditModelData: Missing required field: coefficients";
        return std::nullopt;
    }
    if (!j["coefficients"].is_object()) {
        error = "BanditModelData: Field 'coefficients' must be an object";
        return std::nullopt;
    }

    BanditModelData bmd;
    bmd.gamma = *gamma;
    bmd.defaultActionScore = *defaultActionScore;
    bmd.actionProbabilityFloor = *actionProbabilityFloor;

    for (auto& [actionKey, coeffJson] : j["coefficients"].items()) {
        TRY_PARSE(coefficients, parseBanditCoefficients, coeffJson,
                  "BanditModelData: Invalid coefficients for action '" + actionKey + "': ", error);
        bmd.coefficients[actionKey] = *coefficients;
    }

    return bmd;
}

std::optional<BanditConfiguration> parseBanditConfiguration(const nlohmann::json& j,
                                                            std::string& error) {
    TRY_GET_REQUIRED(banditKey, std::string, j, "banditKey", "BanditConfiguration", error);
    TRY_GET_REQUIRED(modelName, std::string, j, "modelName", "BanditConfiguration", error);
    TRY_GET_REQUIRED(modelVersion, std::string, j, "modelVersion", "BanditConfiguration", error);

    // Check modelData exists before parsing
    if (!j.contains("modelData")) {
        error = "BanditConfiguration: Missing required field: modelData";
        return std::nullopt;
    }

    TRY_PARSE(modelData, parseBanditModelData, j["modelData"],
              "BanditConfiguration: Invalid modelData: ", error);

    BanditConfiguration bc;
    bc.banditKey = *banditKey;
    bc.modelName = *modelName;
    bc.modelVersion = *modelVersion;
    bc.modelData = *modelData;

    if (j.contains("updatedAt") && j["updatedAt"].is_string()) {
        bc.updatedAt = parseISOTimestamp(j["updatedAt"].get_ref<const std::string&>(), error);
        if (!error.empty()) {
            error = "BanditConfiguration: Invalid updatedAt: " + error;
        }
    }

    return bc;
}

std::optional<BanditVariation> parseBanditVariation(const nlohmann::json& j, std::string& error) {
    TRY_GET_REQUIRED(key, std::string, j, "key", "BanditVariation", error);
    TRY_GET_REQUIRED(flagKey, std::string, j, "flagKey", "BanditVariation", error);
    TRY_GET_REQUIRED(variationKey, std::string, j, "variationKey", "BanditVariation", error);
    TRY_GET_REQUIRED(variationValue, std::string, j, "variationValue", "BanditVariation", error);

    BanditVariation bv;
    bv.key = *key;
    bv.flagKey = *flagKey;
    bv.variationKey = *variationKey;
    bv.variationValue = *variationValue;
    return bv;
}

}  // namespace internal

// BanditNumericAttributeCoefficient serialization
void to_json(nlohmann::json& j, const BanditNumericAttributeCoefficient& bnac) {
    j = nlohmann::json{{"attributeKey", bnac.attributeKey},
                       {"coefficient", bnac.coefficient},
                       {"missingValueCoefficient", bnac.missingValueCoefficient}};
}

// BanditCategoricalAttributeCoefficient serialization
void to_json(nlohmann::json& j, const BanditCategoricalAttributeCoefficient& bcac) {
    j = nlohmann::json{{"attributeKey", bcac.attributeKey},
                       {"missingValueCoefficient", bcac.missingValueCoefficient},
                       {"valueCoefficients", bcac.valueCoefficients}};
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

// BanditModelData serialization
void to_json(nlohmann::json& j, const BanditModelData& bmd) {
    j = nlohmann::json{{"gamma", bmd.gamma},
                       {"defaultActionScore", bmd.defaultActionScore},
                       {"actionProbabilityFloor", bmd.actionProbabilityFloor},
                       {"coefficients", bmd.coefficients}};
}

// BanditConfiguration serialization
void to_json(nlohmann::json& j, const BanditConfiguration& bc) {
    j = nlohmann::json{{"banditKey", bc.banditKey},
                       {"modelName", bc.modelName},
                       {"modelVersion", bc.modelVersion},
                       {"modelData", bc.modelData},
                       {"updatedAt", formatISOTimestamp(bc.updatedAt)}};
}

// BanditResponse serialization
void to_json(nlohmann::json& j, const BanditResponse& br) {
    j = nlohmann::json{{"bandits", br.bandits}, {"updatedAt", formatISOTimestamp(br.updatedAt)}};
}

namespace internal {

BanditResponse parseBanditResponse(const std::string& banditJson, std::string& error) {
    error.clear();
    BanditResponse response;

    // Use the non-throwing version of parse (returns discarded value on error)
    nlohmann::json j = nlohmann::json::parse(banditJson, nullptr, false);

    if (j.is_discarded()) {
        error = "Failed to parse JSON bandit response string";
        return BanditResponse();  // Return empty BanditResponse on error
    }

    std::vector<std::string> allErrors;

    // Parse bandits - each bandit independently, collect errors
    if (j.contains("bandits") && j["bandits"].is_object()) {
        for (auto& [banditKey, banditJson] : j["bandits"].items()) {
            std::string parseError;
            auto bandit = internal::parseBanditConfiguration(banditJson, parseError);

            if (bandit) {
                response.bandits[banditKey] = *bandit;
            } else if (!parseError.empty()) {
                allErrors.push_back("Bandit '" + banditKey + "': " + parseError);
            }
        }
    }

    // Parse updatedAt field
    if (j.contains("updatedAt") && j["updatedAt"].is_string()) {
        std::string parseError;
        response.updatedAt =
            parseISOTimestamp(j["updatedAt"].get_ref<const std::string&>(), parseError);
        if (!parseError.empty()) {
            allErrors.push_back("Invalid updatedAt: " + parseError);
        }
    }

    // Consolidate all errors into a single error message
    if (!allErrors.empty()) {
        error = "Bandit response parsing encountered errors:\n";
        for (const auto& err : allErrors) {
            error += "  - " + err + "\n";
        }
    }

    return response;
}

}  // namespace internal

// BanditVariation serialization
void to_json(nlohmann::json& j, const BanditVariation& bv) {
    j = nlohmann::json{{"key", bv.key},
                       {"flagKey", bv.flagKey},
                       {"variationKey", bv.variationKey},
                       {"variationValue", bv.variationValue}};
}

}  // namespace eppoclient

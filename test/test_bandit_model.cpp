#include <catch_amalgamated.hpp>
#include <nlohmann/json.hpp>
#include "../src/bandit_model.hpp"

using namespace eppoclient;
using json = nlohmann::json;

TEST_CASE("BanditNumericAttributeCoefficient serialization", "[bandit_model]") {
    // Create a coefficient
    BanditNumericAttributeCoefficient coeff;
    coeff.attributeKey = "age";
    coeff.coefficient = 0.5;
    coeff.missingValueCoefficient = 0.1;

    // Serialize to JSON
    json j = coeff;

    // Check JSON content
    CHECK(j["attributeKey"] == "age");
    CHECK(j["coefficient"] == 0.5);
    CHECK(j["missingValueCoefficient"] == 0.1);

    // Deserialize from JSON
    std::string error;
    auto maybeCoeff2 = internal::parseBanditNumericAttributeCoefficient(j, error);
    REQUIRE(maybeCoeff2.has_value());
    BanditNumericAttributeCoefficient coeff2 = *maybeCoeff2;
    CHECK(coeff2.attributeKey == "age");
    CHECK(coeff2.coefficient == 0.5);
    CHECK(coeff2.missingValueCoefficient == 0.1);
}

TEST_CASE("BanditCategoricalAttributeCoefficient serialization", "[bandit_model]") {
    // Create a coefficient
    BanditCategoricalAttributeCoefficient coeff;
    coeff.attributeKey = "country";
    coeff.missingValueCoefficient = 0.0;
    coeff.valueCoefficients["US"] = 1.0;
    coeff.valueCoefficients["UK"] = 0.8;
    coeff.valueCoefficients["CA"] = 0.9;

    // Serialize to JSON
    json j = coeff;

    // Check JSON content
    CHECK(j["attributeKey"] == "country");
    CHECK(j["missingValueCoefficient"] == 0.0);
    CHECK(j["valueCoefficients"]["US"] == 1.0);
    CHECK(j["valueCoefficients"]["UK"] == 0.8);
    CHECK(j["valueCoefficients"]["CA"] == 0.9);

    // Deserialize from JSON
    std::string error;
    auto maybeCoeff2 = internal::parseBanditCategoricalAttributeCoefficient(j, error);
    REQUIRE(maybeCoeff2.has_value());
    BanditCategoricalAttributeCoefficient coeff2 = *maybeCoeff2;
    CHECK(coeff2.attributeKey == "country");
    CHECK(coeff2.missingValueCoefficient == 0.0);
    CHECK(coeff2.valueCoefficients["US"] == 1.0);
    CHECK(coeff2.valueCoefficients["UK"] == 0.8);
    CHECK(coeff2.valueCoefficients["CA"] == 0.9);
}

TEST_CASE("BanditCoefficients serialization", "[bandit_model]") {
    // Create coefficients
    BanditCoefficients coeff;
    coeff.actionKey = "action1";
    coeff.intercept = 1.5;

    BanditNumericAttributeCoefficient numCoeff;
    numCoeff.attributeKey = "age";
    numCoeff.coefficient = 0.3;
    numCoeff.missingValueCoefficient = 0.0;
    coeff.subjectNumericCoefficients.push_back(numCoeff);

    BanditCategoricalAttributeCoefficient catCoeff;
    catCoeff.attributeKey = "country";
    catCoeff.missingValueCoefficient = 0.0;
    catCoeff.valueCoefficients["US"] = 0.5;
    coeff.subjectCategoricalCoefficients.push_back(catCoeff);

    // Serialize to JSON
    json j = coeff;

    // Check JSON content
    CHECK(j["actionKey"] == "action1");
    CHECK(j["intercept"] == 1.5);
    CHECK(j["subjectNumericCoefficients"].size() == 1);
    CHECK(j["subjectCategoricalCoefficients"].size() == 1);

    // Deserialize from JSON
    std::string error;
    auto maybeCoeff2 = internal::parseBanditCoefficients(j, error);
    REQUIRE(maybeCoeff2.has_value());
    BanditCoefficients coeff2 = *maybeCoeff2;
    CHECK(coeff2.actionKey == "action1");
    CHECK(coeff2.intercept == 1.5);
    CHECK(coeff2.subjectNumericCoefficients.size() == 1);
    CHECK(coeff2.subjectCategoricalCoefficients.size() == 1);
}

TEST_CASE("BanditModelData serialization", "[bandit_model]") {
    // Create model data
    BanditModelData modelData;
    modelData.gamma = 0.9;
    modelData.defaultActionScore = 0.5;
    modelData.actionProbabilityFloor = 0.01;

    BanditCoefficients coeff;
    coeff.actionKey = "action1";
    coeff.intercept = 1.0;
    modelData.coefficients["action1"] = coeff;

    // Serialize to JSON
    json j = modelData;

    // Check JSON content
    CHECK(j["gamma"] == 0.9);
    CHECK(j["defaultActionScore"] == 0.5);
    CHECK(j["actionProbabilityFloor"] == 0.01);
    CHECK(j["coefficients"].size() == 1);

    // Deserialize from JSON
    std::string error;
    auto maybeModelData2 = internal::parseBanditModelData(j, error);
    REQUIRE(maybeModelData2.has_value());
    BanditModelData modelData2 = *maybeModelData2;
    CHECK(modelData2.gamma == 0.9);
    CHECK(modelData2.defaultActionScore == 0.5);
    CHECK(modelData2.actionProbabilityFloor == 0.01);
    CHECK(modelData2.coefficients.size() == 1);
}

TEST_CASE("BanditConfiguration serialization", "[bandit_model]") {
    // Create configuration
    BanditConfiguration config;
    config.banditKey = "my-bandit";
    config.modelName = "contextual";
    config.modelVersion = "v1";
    config.modelData.gamma = 0.8;

    // Serialize to JSON
    json j = config;

    // Check JSON content
    CHECK(j["banditKey"] == "my-bandit");
    CHECK(j["modelName"] == "contextual");
    CHECK(j["modelVersion"] == "v1");

    // Deserialize from JSON
    std::string error;
    auto maybeConfig2 = internal::parseBanditConfiguration(j, error);
    REQUIRE(maybeConfig2.has_value());
    BanditConfiguration config2 = *maybeConfig2;
    CHECK(config2.banditKey == "my-bandit");
    CHECK(config2.modelName == "contextual");
    CHECK(config2.modelVersion == "v1");
    CHECK(config2.modelData.gamma == 0.8);
}

TEST_CASE("BanditVariation serialization", "[bandit_model]") {
    // Create variation
    BanditVariation variation;
    variation.key = "var1";
    variation.flagKey = "my-flag";
    variation.variationKey = "control";
    variation.variationValue = "off";

    // Serialize to JSON
    json j = variation;

    // Check JSON content
    CHECK(j["key"] == "var1");
    CHECK(j["flagKey"] == "my-flag");
    CHECK(j["variationKey"] == "control");
    CHECK(j["variationValue"] == "off");

    // Deserialize from JSON
    std::string error;
    auto maybeVariation2 = internal::parseBanditVariation(j, error);
    REQUIRE(maybeVariation2.has_value());
    BanditVariation variation2 = *maybeVariation2;
    CHECK(variation2.key == "var1");
    CHECK(variation2.flagKey == "my-flag");
    CHECK(variation2.variationKey == "control");
    CHECK(variation2.variationValue == "off");
}

TEST_CASE("BanditResponse serialization", "[bandit_model]") {
    // Create response
    BanditResponse response;

    BanditConfiguration config;
    config.banditKey = "bandit1";
    config.modelName = "contextual";
    config.modelVersion = "v1";
    config.modelData.gamma = 0.85;

    response.bandits["bandit1"] = config;

    // Serialize to JSON
    json j = response;

    // Check JSON content
    CHECK(j["bandits"].size() == 1);
    CHECK(j["bandits"]["bandit1"]["banditKey"] == "bandit1");

    // Deserialize from JSON
    std::string error2;
    BanditResponse response2 = parseBanditResponse(j.dump(), error2);
    CHECK(error2.empty());
    CHECK(response2.bandits.size() == 1);
    CHECK(response2.bandits["bandit1"].banditKey == "bandit1");
    CHECK(response2.bandits["bandit1"].modelData.gamma == 0.85);
}

TEST_CASE("Complete JSON round-trip", "[bandit_model]") {
    // Create a complete bandit configuration JSON similar to what might come from the API
    std::string jsonStr = R"({
        "bandits": {
            "recommendation-bandit": {
                "banditKey": "recommendation-bandit",
                "modelName": "falcon",
                "modelVersion": "v123",
                "updatedAt": "2024-01-15T10:30:00Z",
                "modelData": {
                    "gamma": 1.0,
                    "defaultActionScore": 0.0,
                    "actionProbabilityFloor": 0.0,
                    "coefficients": {
                        "action1": {
                            "actionKey": "action1",
                            "intercept": 1.5,
                            "subjectNumericCoefficients": [
                                {
                                    "attributeKey": "age",
                                    "coefficient": 0.05,
                                    "missingValueCoefficient": 0.0
                                }
                            ],
                            "subjectCategoricalCoefficients": [
                                {
                                    "attributeKey": "country",
                                    "missingValueCoefficient": 0.0,
                                    "valueCoefficients": {
                                        "US": 1.2,
                                        "UK": 0.8
                                    }
                                }
                            ],
                            "actionNumericCoefficients": [],
                            "actionCategoricalCoefficients": []
                        }
                    }
                }
            }
        },
        "updatedAt": "2024-01-15T10:30:00Z"
    })";

    // Deserialize to BanditResponse
    std::string error;
    BanditResponse response = parseBanditResponse(jsonStr, error);
    CHECK(error.empty());

    // Verify structure
    CHECK(response.bandits.size() == 1);
    CHECK(response.bandits.count("recommendation-bandit") == 1);

    auto& config = response.bandits["recommendation-bandit"];
    CHECK(config.banditKey == "recommendation-bandit");
    CHECK(config.modelName == "falcon");
    CHECK(config.modelVersion == "v123");
    CHECK(config.modelData.gamma == 1.0);
    CHECK(config.modelData.coefficients.size() == 1);

    auto& action1 = config.modelData.coefficients["action1"];
    CHECK(action1.actionKey == "action1");
    CHECK(action1.intercept == 1.5);
    CHECK(action1.subjectNumericCoefficients.size() == 1);
    CHECK(action1.subjectCategoricalCoefficients.size() == 1);

    // Serialize back to JSON
    json j2 = response;

    // Verify round-trip
    CHECK(j2["bandits"]["recommendation-bandit"]["banditKey"] == "recommendation-bandit");
    CHECK(j2["bandits"]["recommendation-bandit"]["modelData"]["gamma"] == 1.0);
}

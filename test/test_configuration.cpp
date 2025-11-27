#include <catch_amalgamated.hpp>
#include <nlohmann/json.hpp>
#include "../src/configuration.hpp"

using namespace eppoclient;
using json = nlohmann::json;

TEST_CASE("Configuration getFlagConfiguration", "[configuration]") {
    Configuration config;

    const FlagConfiguration* flag = config.getFlagConfiguration("test-flag");

    // Should return nullptr since we haven't set any configuration
    CHECK(flag == nullptr);
}

TEST_CASE("Configuration getBanditConfiguration", "[configuration]") {
    Configuration config;

    const BanditConfiguration* bandit = config.getBanditConfiguration("test-bandit");

    // Should return nullptr since we haven't set any configuration
    CHECK(bandit == nullptr);
}

TEST_CASE("Configuration getBanditVariant", "[configuration]") {
    Configuration config;
    BanditVariation result;

    bool found = config.getBanditVariant("test-flag", "control", result);

    // Should return false since we haven't set any configuration
    CHECK(!found);
}


TEST_CASE("ConfigResponse with bandits", "[configuration]") {
    // Create a complete config response with both flags and bandits
    std::string jsonStr = R"({
        "flags": {
            "recommendation-flag": {
                "key": "recommendation-flag",
                "enabled": true,
                "variationType": "STRING",
                "variations": {
                    "control": {
                        "key": "control",
                        "value": "default-algo"
                    },
                    "bandit": {
                        "key": "bandit",
                        "value": "ml-algo"
                    }
                },
                "allocations": [],
                "totalShards": 10000
            }
        },
        "bandits": {
            "bandit-var-1": [{
                "key": "bandit-var-1",
                "flagKey": "recommendation-flag",
                "variationKey": "bandit",
                "variationValue": "ml-algo"
            }]
        }
    })";

    // Parse the JSON
    json j = json::parse(jsonStr);

    // Deserialize to ConfigResponse
    ConfigResponse response = j;

    // Verify structure
    CHECK(response.flags.size() == 1);
    CHECK(response.flags.count("recommendation-flag") == 1);
    CHECK(response.bandits.size() == 1);
    CHECK(response.bandits.count("bandit-var-1") == 1);

    // Verify flag data
    const auto& flag = response.flags["recommendation-flag"];
    CHECK(flag.key == "recommendation-flag");
    CHECK(flag.enabled == true);
    CHECK(flag.variations.size() == 2);

    // Verify bandit data
    const auto& banditVec = response.bandits["bandit-var-1"];
    CHECK(banditVec.size() == 1);
    const auto& bandit = banditVec[0];
    CHECK(bandit.key == "bandit-var-1");
    CHECK(bandit.flagKey == "recommendation-flag");
    CHECK(bandit.variationKey == "bandit");
    CHECK(bandit.variationValue == "ml-algo");

    // Serialize back to JSON
    json j2 = response;

    // Verify round-trip
    CHECK(j2["flags"]["recommendation-flag"]["key"] == "recommendation-flag");
    CHECK(j2["bandits"]["bandit-var-1"][0]["flagKey"] == "recommendation-flag");
}

TEST_CASE("BanditResponse with multiple bandits", "[configuration]") {
    std::string jsonStr = R"({
        "bandits": {
            "bandit-1": {
                "banditKey": "bandit-1",
                "modelName": "falcon",
                "modelVersion": "v1",
                "updatedAt": "2024-01-15T10:30:00Z",
                "modelData": {
                    "gamma": 1.0,
                    "defaultActionScore": 0.0,
                    "actionProbabilityFloor": 0.0,
                    "coefficients": {
                        "action1": {
                            "actionKey": "action1",
                            "intercept": 1.5,
                            "subjectNumericCoefficients": [],
                            "subjectCategoricalCoefficients": [],
                            "actionNumericCoefficients": [],
                            "actionCategoricalCoefficients": []
                        }
                    }
                }
            },
            "bandit-2": {
                "banditKey": "bandit-2",
                "modelName": "contextual",
                "modelVersion": "v2",
                "updatedAt": "2024-01-15T11:00:00Z",
                "modelData": {
                    "gamma": 0.9,
                    "defaultActionScore": 0.1,
                    "actionProbabilityFloor": 0.05,
                    "coefficients": {}
                }
            }
        },
        "updatedAt": "2024-01-15T11:00:00Z"
    })";

    // Parse JSON
    json j = json::parse(jsonStr);

    // Deserialize to BanditResponse
    BanditResponse response = j;

    // Verify structure
    CHECK(response.bandits.size() == 2);
    CHECK(response.bandits.count("bandit-1") == 1);
    CHECK(response.bandits.count("bandit-2") == 1);

    // Verify bandit-1
    const auto& bandit1 = response.bandits["bandit-1"];
    CHECK(bandit1.banditKey == "bandit-1");
    CHECK(bandit1.modelName == "falcon");
    CHECK(bandit1.modelVersion == "v1");
    CHECK(bandit1.modelData.gamma == 1.0);
    CHECK(bandit1.modelData.coefficients.size() == 1);

    // Verify bandit-2
    const auto& bandit2 = response.bandits["bandit-2"];
    CHECK(bandit2.banditKey == "bandit-2");
    CHECK(bandit2.modelName == "contextual");
    CHECK(bandit2.modelVersion == "v2");
    CHECK(bandit2.modelData.gamma == 0.9);
    CHECK(bandit2.modelData.coefficients.size() == 0);

    // Serialize back
    json j2 = response;

    // Verify round-trip
    CHECK(j2["bandits"]["bandit-1"]["modelName"] == "falcon");
    CHECK(j2["bandits"]["bandit-2"]["modelName"] == "contextual");
}

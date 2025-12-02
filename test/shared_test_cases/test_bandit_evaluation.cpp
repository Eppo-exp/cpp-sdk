#include <catch_amalgamated.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include "../src/bandit_model.hpp"
#include "../src/client.hpp"
#include "../src/config_response.hpp"

using namespace eppoclient;
using json = nlohmann::json;
namespace fs = std::filesystem;

// Structure to hold action data for bandit tests
struct TestAction {
    std::string actionKey;
    ContextAttributes attributes;
};

// Structure to hold test subject data for bandit tests
struct BanditTestSubject {
    std::string subjectKey;
    ContextAttributes subjectAttributes;
    std::vector<TestAction> actions;
    std::string expectedVariation;
    std::optional<std::string> expectedAction;  // Can be null
};

// Structure to hold bandit test case data
struct BanditTestCase {
    std::string flag;
    std::string defaultValue;
    std::vector<BanditTestSubject> subjects;
    std::string filename;  // For better error messages
};

// Helper function to parse context attributes from JSON
// This handles "dynamic typing" test cases where values may have incorrect types
ContextAttributes parseContextAttributes(const json& attrJson) {
    ContextAttributes attributes;

    if (attrJson.contains("numericAttributes") && attrJson["numericAttributes"].is_object()) {
        for (auto it = attrJson["numericAttributes"].begin();
             it != attrJson["numericAttributes"].end(); ++it) {
            std::string key = it.key();
            const auto& value = it.value();

            // Only accept actual numbers, skip strings, booleans, nulls, etc.
            if (value.is_number()) {
                attributes.numericAttributes[key] = value.get<double>();
            }
            // For dynamic typing tests: silently skip non-numeric values
        }
    }

    if (attrJson.contains("categoricalAttributes") &&
        attrJson["categoricalAttributes"].is_object()) {
        for (auto it = attrJson["categoricalAttributes"].begin();
             it != attrJson["categoricalAttributes"].end(); ++it) {
            std::string key = it.key();
            const auto& value = it.value();

            // Accept strings directly
            if (value.is_string()) {
                attributes.categoricalAttributes[key] = value.get<std::string>();
            }
            // For dynamic typing tests: convert numbers to strings
            else if (value.is_number_integer()) {
                attributes.categoricalAttributes[key] = std::to_string(value.get<int64_t>());
            } else if (value.is_number_float()) {
                attributes.categoricalAttributes[key] = std::to_string(value.get<double>());
            }
            // Skip nulls, booleans, objects, arrays
        }
    }

    return attributes;
}

// Helper function to load bandit flags configuration from JSON file
ConfigResponse loadBanditFlagsConfiguration(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open bandit flags configuration file: " + filepath);
    }

    json j;
    file >> j;

    auto result = parseConfigResponse(j);
    if (!result.hasValue() || result.hasErrors()) {
        throw std::runtime_error("Failed to parse config: " +
                                 (result.errors.empty() ? "unknown error" : result.errors[0]));
    }
    return *result.value;
}

// Helper function to load bandit models configuration from JSON file
BanditResponse loadBanditModelsConfiguration(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open bandit models configuration file: " + filepath);
    }

    json j;
    file >> j;

    auto result = parseBanditResponse(j);
    if (!result.hasValue() || result.hasErrors()) {
        throw std::runtime_error("Failed to parse bandit response: " +
                                 (result.errors.empty() ? "unknown error" : result.errors[0]));
    }
    return *result.value;
}

// Helper function to load a single bandit test case from JSON file
BanditTestCase loadBanditTestCase(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open bandit test case file: " + filepath);
    }

    json j;
    file >> j;

    BanditTestCase testCase;
    testCase.flag = j["flag"].get<std::string>();
    testCase.defaultValue = j["defaultValue"].get<std::string>();
    testCase.filename = fs::path(filepath).filename().string();

    // Parse subjects
    for (const auto& subjectJson : j["subjects"]) {
        BanditTestSubject subject;
        subject.subjectKey = subjectJson["subjectKey"].get<std::string>();
        subject.subjectAttributes = parseContextAttributes(subjectJson["subjectAttributes"]);

        // Parse actions
        if (subjectJson.contains("actions") && subjectJson["actions"].is_array()) {
            for (const auto& actionJson : subjectJson["actions"]) {
                TestAction action;
                action.actionKey = actionJson["actionKey"].get<std::string>();
                action.attributes = parseContextAttributes(actionJson);
                subject.actions.push_back(action);
            }
        }

        // Parse expected assignment
        const auto& assignment = subjectJson["assignment"];
        subject.expectedVariation = assignment["variation"].get<std::string>();

        if (assignment["action"].is_null()) {
            subject.expectedAction = std::nullopt;
        } else {
            subject.expectedAction = assignment["action"].get<std::string>();
        }

        testCase.subjects.push_back(subject);
    }

    return testCase;
}

// Helper function to load all bandit test cases from directory
std::vector<BanditTestCase> loadAllBanditTestCases(const std::string& directory) {
    std::vector<BanditTestCase> testCases;

    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            std::string filename = entry.path().filename().string();
            // Skip files that don't start with "test-"
            if (filename.find("test-") == 0) {
                try {
                    testCases.push_back(loadBanditTestCase(entry.path().string()));
                } catch (const std::exception& e) {
                    std::cerr << "Warning: Failed to load bandit test case " << filename << ": "
                              << e.what() << std::endl;
                }
            }
        }
    }

    return testCases;
}

/**
 * Bandit Evaluation Tests
 *
 * These tests load bandit configuration from test/data/ufc/bandit-flags-v1.json
 * and test/data/ufc/bandit-models-v1.json, then run all test cases from
 * test/data/ufc/bandit-tests/ directory (all .json files).
 *
 * The tests verify that the bandit evaluation algorithm correctly:
 * 1. Gets the flag variation
 * 2. Finds the associated bandit configuration
 * 3. Evaluates the bandit model to select an action
 * 4. Logs the bandit event with proper attributes
 *
 * Note: The bandit-flags-v1.json file has a complex structure where bandits are
 * stored as arrays. We flatten this structure before passing to ConfigResponse.
 */
TEST_CASE("UFC Bandit Test Cases - Bandit Action Selection", "[ufc][bandits]") {
    // Load the JSON files directly
    std::string flagsPath = "test/data/ufc/bandit-flags-v1.json";
    std::string modelsPath = "test/data/ufc/bandit-models-v1.json";

    // Read flags JSON
    std::ifstream flagsFile(flagsPath);
    REQUIRE(flagsFile.is_open());
    json flagsJson;
    flagsFile >> flagsJson;
    flagsFile.close();

    // Read models JSON
    std::ifstream modelsFile(modelsPath);
    REQUIRE(modelsFile.is_open());
    json modelsJson;
    modelsFile >> modelsJson;
    modelsFile.close();

    // Create a JSON for ConfigResponse with flags and flattened bandits
    json configJson;
    configJson["flags"] = flagsJson["flags"];

    // Parse the bandits array structure - ConfigResponse expects arrays
    // The bandit-flags-v1.json has bandits as arrays which matches our structure
    if (flagsJson.contains("bandits")) {
        configJson["bandits"] = flagsJson["bandits"];
    }

    // Create ConfigResponse from the flags JSON
    auto configResult = parseConfigResponse(configJson);
    if (!configResult.hasValue() || configResult.hasErrors()) {
        FAIL("Failed to parse config: " +
             (configResult.errors.empty() ? "unknown error" : configResult.errors[0]));
    }
    ConfigResponse configResponse = *configResult.value;

    // Create BanditResponse from the models JSON
    auto banditResult = parseBanditResponse(modelsJson);
    if (!banditResult.hasValue() || banditResult.hasErrors()) {
        FAIL("Failed to parse bandit response: " +
             (banditResult.errors.empty() ? "unknown error" : banditResult.errors[0]));
    }
    BanditResponse banditResponse = *banditResult.value;

    // Create configuration with both flags and bandit models
    Configuration combinedConfig(configResponse, banditResponse);

    // Create client with configuration
    auto configStore = std::make_shared<ConfigurationStore>();
    configStore->setConfiguration(combinedConfig);

    // Mock bandit logger to verify logging
    class MockBanditLogger : public BanditLogger {
    public:
        std::vector<BanditEvent> loggedEvents;

        void logBanditAction(const BanditEvent& event) override { loggedEvents.push_back(event); }
    };

    auto banditLogger = std::make_shared<MockBanditLogger>();
    EppoClient client(configStore, nullptr, banditLogger, nullptr);

    // Load all bandit test cases
    std::string testCasesDir = "test/data/ufc/bandit-tests";
    std::vector<BanditTestCase> testCases;

    REQUIRE_NOTHROW(testCases = loadAllBanditTestCases(testCasesDir));
    REQUIRE(testCases.size() > 0);

    // Run through each test case
    for (const auto& testCase : testCases) {
        DYNAMIC_SECTION("Test case: " << testCase.filename << " - Flag: " << testCase.flag) {
            // Run through each subject in the test case
            for (size_t i = 0; i < testCase.subjects.size(); i++) {
                const auto& subject = testCase.subjects[i];

                DYNAMIC_SECTION("Subject " << i << ": " << subject.subjectKey) {
                    // Convert actions to map format expected by getBanditAction
                    std::map<std::string, ContextAttributes> actionsMap;
                    for (const auto& action : subject.actions) {
                        actionsMap[action.actionKey] = action.attributes;
                    }

                    // Clear logged events before test
                    banditLogger->loggedEvents.clear();

                    // Get bandit action
                    BanditResult result = client.getBanditAction(testCase.flag, subject.subjectKey,
                                                                 subject.subjectAttributes,
                                                                 actionsMap, testCase.defaultValue);

                    // Verify the variation matches expected value
                    INFO("Flag: " << testCase.flag);
                    INFO("Subject: " << subject.subjectKey);
                    INFO("Expected variation: " << subject.expectedVariation);
                    INFO("Actual variation: " << result.variation);
                    CHECK(result.variation == subject.expectedVariation);

                    // Verify the action matches expected value
                    if (subject.expectedAction.has_value()) {
                        INFO("Expected action: " << subject.expectedAction.value());
                        REQUIRE(result.action.has_value());
                        INFO("Actual action: " << result.action.value());
                        CHECK(result.action.value() == subject.expectedAction.value());

                        // Verify that bandit action was logged when an action is selected
                        CHECK(banditLogger->loggedEvents.size() == 1);

                        if (!banditLogger->loggedEvents.empty()) {
                            const auto& loggedEvent = banditLogger->loggedEvents[0];
                            CHECK(loggedEvent.flagKey == testCase.flag);
                            CHECK(loggedEvent.subject == subject.subjectKey);
                            CHECK(loggedEvent.action == subject.expectedAction.value());
                            CHECK(loggedEvent.metaData.at("sdkLanguage") == "cpp");
                        }
                    } else {
                        INFO("Expected action: null");
                        INFO("Actual action: " << (result.action.has_value() ? result.action.value()
                                                                             : "null"));
                        CHECK(!result.action.has_value());

                        // Verify that no bandit action was logged when action is null
                        CHECK(banditLogger->loggedEvents.size() == 0);
                    }
                }
            }
        }
    }
}

// Test to verify we can load the bandit flags configuration
TEST_CASE("Load bandit flags configuration", "[ufc][bandit-config]") {
    std::string flagsPath = "test/data/ufc/bandit-flags-v1.json";

    SECTION("Bandit flags file exists and can be parsed") {
        std::ifstream file(flagsPath);
        REQUIRE(file.is_open());

        json j;
        REQUIRE_NOTHROW(file >> j);
        file.close();

        // Verify the JSON has the expected structure
        REQUIRE(j.contains("flags"));
        REQUIRE(j["flags"].is_object());
        REQUIRE(j["flags"].size() > 0);

        // Check for specific known flags
        CHECK(j["flags"].contains("banner_bandit_flag"));
    }
}

// Test to verify we can load the bandit models configuration
TEST_CASE("Load bandit models configuration", "[ufc][bandit-config]") {
    std::string modelsPath = "test/data/ufc/bandit-models-v1.json";

    SECTION("Bandit models file exists and can be parsed") {
        BanditResponse response;
        REQUIRE_NOTHROW(response = loadBanditModelsConfiguration(modelsPath));

        // Verify we have some bandits
        REQUIRE(response.bandits.size() > 0);

        // Check for specific known bandits
        CHECK(response.bandits.count("banner_bandit") > 0);

        // Verify model structure
        if (response.bandits.count("banner_bandit") > 0) {
            const auto& bandit = response.bandits["banner_bandit"];
            CHECK(bandit.banditKey == "banner_bandit");
            CHECK(bandit.modelName == "falcon");
            CHECK(!bandit.modelVersion.empty());
            CHECK(bandit.modelData.coefficients.size() > 0);
        }
    }
}

// Test to verify we can load all bandit test cases
TEST_CASE("Load all bandit test cases", "[ufc][bandit-testcases]") {
    std::string testCasesDir = "test/data/ufc/bandit-tests";

    SECTION("Bandit test cases directory exists and contains test files") {
        std::vector<BanditTestCase> testCases;
        REQUIRE_NOTHROW(testCases = loadAllBanditTestCases(testCasesDir));

        // Verify we have some test cases
        REQUIRE(testCases.size() > 0);

        // Print out loaded test cases for debugging
        std::cout << "Loaded " << testCases.size() << " bandit test cases:" << std::endl;
        for (const auto& tc : testCases) {
            std::cout << "  - " << tc.filename << " (flag: " << tc.flag
                      << ", subjects: " << tc.subjects.size() << ")" << std::endl;
        }
    }
}

// Test ContextAttributes helper functions
TEST_CASE("ContextAttributes conversion functions", "[bandits][helpers]") {
    SECTION("inferContextAttributes converts Attributes to ContextAttributes") {
        Attributes attrs;
        attrs["age"] = int64_t(25);
        attrs["price"] = 19.99;
        attrs["country"] = std::string("USA");
        attrs["is_active"] = true;
        attrs["null_value"] = std::monostate{};

        ContextAttributes contextAttrs = inferContextAttributes(attrs);

        CHECK(contextAttrs.numericAttributes.size() == 2);
        CHECK(contextAttrs.numericAttributes["age"] == 25.0);
        CHECK(contextAttrs.numericAttributes["price"] == 19.99);

        CHECK(contextAttrs.categoricalAttributes.size() == 2);
        CHECK(contextAttrs.categoricalAttributes["country"] == "USA");
        CHECK(contextAttrs.categoricalAttributes["is_active"] == "true");
    }

    SECTION("toGenericAttributes converts ContextAttributes to Attributes") {
        ContextAttributes contextAttrs;
        contextAttrs.numericAttributes["age"] = 30.0;
        contextAttrs.numericAttributes["score"] = 95.5;
        contextAttrs.categoricalAttributes["tier"] = "gold";
        contextAttrs.categoricalAttributes["region"] = "west";

        Attributes attrs = toGenericAttributes(contextAttrs);

        CHECK(attrs.size() == 4);
        CHECK(std::holds_alternative<double>(attrs["age"]));
        CHECK(std::get<double>(attrs["age"]) == 30.0);
        CHECK(std::holds_alternative<double>(attrs["score"]));
        CHECK(std::get<double>(attrs["score"]) == 95.5);
        CHECK(std::holds_alternative<std::string>(attrs["tier"]));
        CHECK(std::get<std::string>(attrs["tier"]) == "gold");
        CHECK(std::holds_alternative<std::string>(attrs["region"]));
        CHECK(std::get<std::string>(attrs["region"]) == "west");
    }

    SECTION("Round-trip conversion preserves data") {
        Attributes original;
        original["count"] = int64_t(42);
        original["ratio"] = 0.75;
        original["name"] = std::string("test");
        original["enabled"] = false;

        ContextAttributes contextAttrs = inferContextAttributes(original);
        Attributes converted = toGenericAttributes(contextAttrs);

        // Note: int64_t is converted to double in the round-trip
        CHECK(converted.size() == 4);
        CHECK(std::get<double>(converted["count"]) == 42.0);
        CHECK(std::get<double>(converted["ratio"]) == 0.75);
        CHECK(std::get<std::string>(converted["name"]) == "test");
        CHECK(std::get<std::string>(converted["enabled"]) == "false");
    }
}

// Test edge cases
TEST_CASE("Bandit edge cases", "[bandits][edge-cases]") {
    // Create a simple configuration
    auto configStore = std::make_shared<ConfigurationStore>();
    EppoClient client(configStore, nullptr, nullptr, nullptr);

    SECTION("getBanditAction with empty actions returns variation without action") {
        ContextAttributes subjectAttrs;
        subjectAttrs.numericAttributes["age"] = 25.0;

        std::map<std::string, ContextAttributes> emptyActions;

        BanditResult result = client.getBanditAction("test-flag", "test-subject", subjectAttrs,
                                                     emptyActions, "default");

        // Should return default variation with no action
        CHECK(result.variation == "default");
        CHECK(!result.action.has_value());
    }

    SECTION("BanditResult can be constructed") {
        BanditResult result1("control", "action1");
        CHECK(result1.variation == "control");
        CHECK(result1.action.has_value());
        CHECK(result1.action.value() == "action1");

        BanditResult result2("treatment", std::nullopt);
        CHECK(result2.variation == "treatment");
        CHECK(!result2.action.has_value());
    }
}

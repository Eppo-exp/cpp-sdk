#include <catch_amalgamated.hpp>
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include "../src/client.hpp"
#include "../src/config_response.hpp"

using namespace eppoclient;
using json = nlohmann::json;

namespace {
// Mock application logger
class MockApplicationLogger : public ApplicationLogger {
public:
    std::vector<std::string> debugMessages;
    std::vector<std::string> infoMessages;
    std::vector<std::string> warnMessages;
    std::vector<std::string> errorMessages;

    void debug(const std::string& message) override { debugMessages.push_back(message); }

    void info(const std::string& message) override { infoMessages.push_back(message); }

    void warn(const std::string& message) override { warnMessages.push_back(message); }

    void error(const std::string& message) override { errorMessages.push_back(message); }

    void clear() {
        debugMessages.clear();
        infoMessages.clear();
        warnMessages.clear();
        errorMessages.clear();
    }
};

// Mock assignment logger
class MockAssignmentLogger : public AssignmentLogger {
public:
    std::vector<AssignmentEvent> loggedEvents;

    void logAssignment(const AssignmentEvent& event) override { loggedEvents.push_back(event); }

    void clear() { loggedEvents.clear(); }
};

// Helper function to load flags configuration
ConfigResponse loadFlagsConfiguration(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open flags configuration file: " + filepath);
    }

    json j;
    file >> j;
    return j.get<ConfigResponse>();
}
}  // namespace

TEST_CASE("Assignment Details - getBooleanAssignmentDetails", "[assignment-details]") {
    ConfigurationStore configStore;
    ConfigResponse ufc = loadFlagsConfiguration("test/data/ufc/flags-v1.json");
    configStore.setConfiguration(Configuration(ufc));

    auto mockAppLogger = std::make_shared<MockApplicationLogger>();
    auto mockAssignmentLogger = std::make_shared<MockAssignmentLogger>();

    EppoClient client(configStore, mockAssignmentLogger, nullptr, mockAppLogger);

    SECTION("Successful boolean assignment with details") {
        Attributes attrs;
        attrs["should_disable_feature"] = false;

        auto result = client.getBooleanAssignmentDetails("boolean-false-assignment", "test-subject",
                                                         attrs, false);

        // Check variation value
        CHECK(result.variation == true);

        // Check evaluation details are present
        REQUIRE(result.evaluationDetails.has_value());

        // Check evaluation details content
        CHECK(result.evaluationDetails->flagKey == "boolean-false-assignment");
        CHECK(result.evaluationDetails->subjectKey == "test-subject");
        REQUIRE(result.evaluationDetails->flagEvaluationCode.has_value());
        CHECK(*result.evaluationDetails->flagEvaluationCode == FlagEvaluationCode::MATCH);
        CHECK(result.evaluationDetails->flagEvaluationDescription == "Flag evaluation successful");
        REQUIRE(result.evaluationDetails->variationKey.has_value());
        CHECK(*result.evaluationDetails->variationKey == "true-variation");
    }

    SECTION("Non-existent flag returns default with details") {
        mockAppLogger->clear();

        auto result = client.getBooleanAssignmentDetails("non-existent-flag", "test-subject",
                                                         Attributes(), false);

        // Check default value is returned
        CHECK(result.variation == false);

        // Check evaluation details indicate flag not found
        REQUIRE(result.evaluationDetails.has_value());
        CHECK(result.evaluationDetails->flagKey == "non-existent-flag");
        REQUIRE(result.evaluationDetails->flagEvaluationCode.has_value());
        CHECK(*result.evaluationDetails->flagEvaluationCode ==
              FlagEvaluationCode::FLAG_UNRECOGNIZED_OR_DISABLED);
    }

    SECTION("Empty subject key returns default with error details") {
        auto result =
            client.getBooleanAssignmentDetails("boolean-false-assignment", "", Attributes(), true);

        CHECK(result.variation == true);  // Returns default

        REQUIRE(result.evaluationDetails.has_value());
        REQUIRE(result.evaluationDetails->flagEvaluationCode.has_value());
        CHECK(*result.evaluationDetails->flagEvaluationCode ==
              FlagEvaluationCode::ASSIGNMENT_ERROR);
        CHECK(result.evaluationDetails->flagEvaluationDescription.find("subject key") !=
              std::string::npos);
    }
}

TEST_CASE("Assignment Details - getIntegerAssignmentDetails", "[assignment-details]") {
    ConfigurationStore configStore;
    ConfigResponse ufc = loadFlagsConfiguration("test/data/ufc/flags-v1.json");
    configStore.setConfiguration(Configuration(ufc));

    auto mockAppLogger = std::make_shared<MockApplicationLogger>();
    EppoClient client(configStore, nullptr, nullptr, mockAppLogger);

    SECTION("Successful integer assignment with details") {
        Attributes attrs;
        attrs["age"] = 25.0;

        auto result = client.getIntegerAssignmentDetails("integer-flag", "alice", attrs, 0);

        // Check variation is an integer
        REQUIRE(result.evaluationDetails.has_value());
        CHECK(result.evaluationDetails->flagKey == "integer-flag");
        CHECK(result.evaluationDetails->subjectKey == "alice");
        REQUIRE(result.evaluationDetails->flagEvaluationCode.has_value());
        CHECK(*result.evaluationDetails->flagEvaluationCode == FlagEvaluationCode::MATCH);
        REQUIRE(result.evaluationDetails->variationKey.has_value());
    }

    SECTION("Type mismatch returns default with details") {
        // Try to get integer from a boolean flag
        Attributes attrs;
        attrs["should_disable_feature"] = false;

        auto result = client.getIntegerAssignmentDetails("boolean-false-assignment", "test-subject",
                                                         attrs, 999);

        CHECK(result.variation == 999);  // Returns default

        REQUIRE(result.evaluationDetails.has_value());
        REQUIRE(result.evaluationDetails->flagEvaluationCode.has_value());
        CHECK(*result.evaluationDetails->flagEvaluationCode == FlagEvaluationCode::TYPE_MISMATCH);
    }
}

TEST_CASE("Assignment Details - getNumericAssignmentDetails", "[assignment-details]") {
    ConfigurationStore configStore;
    ConfigResponse ufc = loadFlagsConfiguration("test/data/ufc/flags-v1.json");
    configStore.setConfiguration(Configuration(ufc));

    auto mockAppLogger = std::make_shared<MockApplicationLogger>();
    EppoClient client(configStore, nullptr, nullptr, mockAppLogger);

    SECTION("Successful numeric assignment with details") {
        auto result =
            client.getNumericAssignmentDetails("numeric_flag", "test-subject", Attributes(), 0.0);

        // Check result has details
        REQUIRE(result.evaluationDetails.has_value());
        CHECK(result.evaluationDetails->flagKey == "numeric_flag");
        REQUIRE(result.evaluationDetails->flagEvaluationCode.has_value());
        CHECK(*result.evaluationDetails->flagEvaluationCode == FlagEvaluationCode::MATCH);
        REQUIRE(result.evaluationDetails->variationKey.has_value());
        CHECK(*result.evaluationDetails->variationKey == "pi");

        // Verify the numeric value is pi
        CHECK(result.variation == Catch::Approx(3.14159).epsilon(0.00001));
    }
}

TEST_CASE("Assignment Details - getStringAssignmentDetails", "[assignment-details]") {
    ConfigurationStore configStore;
    ConfigResponse ufc = loadFlagsConfiguration("test/data/ufc/flags-v1.json");
    configStore.setConfiguration(Configuration(ufc));

    auto mockAppLogger = std::make_shared<MockApplicationLogger>();
    EppoClient client(configStore, nullptr, nullptr, mockAppLogger);

    SECTION("Successful string assignment with details") {
        // Use empty_string_flag which is actually a string type
        auto result = client.getStringAssignmentDetails("empty_string_flag", "test-subject",
                                                        Attributes(), "default");

        // Check result has details
        REQUIRE(result.evaluationDetails.has_value());
        CHECK(result.evaluationDetails->flagKey == "empty_string_flag");
        CHECK(result.evaluationDetails->subjectKey == "test-subject");
        REQUIRE(result.evaluationDetails->flagEvaluationCode.has_value());
        CHECK(*result.evaluationDetails->flagEvaluationCode == FlagEvaluationCode::MATCH);
        REQUIRE(result.evaluationDetails->variationKey.has_value());
    }

    SECTION("Non-existent flag returns default with details") {
        auto result = client.getStringAssignmentDetails("non-existent-string-flag", "test-subject",
                                                        Attributes(), "default-value");

        CHECK(result.variation == "default-value");

        REQUIRE(result.evaluationDetails.has_value());
        REQUIRE(result.evaluationDetails->flagEvaluationCode.has_value());
        CHECK(*result.evaluationDetails->flagEvaluationCode ==
              FlagEvaluationCode::FLAG_UNRECOGNIZED_OR_DISABLED);
    }
}

TEST_CASE("Assignment Details - getJsonAssignmentDetails", "[assignment-details]") {
    ConfigurationStore configStore;
    ConfigResponse ufc = loadFlagsConfiguration("test/data/ufc/flags-v1.json");
    configStore.setConfiguration(Configuration(ufc));

    auto mockAppLogger = std::make_shared<MockApplicationLogger>();
    EppoClient client(configStore, nullptr, nullptr, mockAppLogger);

    SECTION("Successful JSON assignment with details") {
        json defaultJson = {{"default", "value"}};

        auto result = client.getJsonAssignmentDetails("json-config-flag", "test-subject-1",
                                                      Attributes(), defaultJson);

        // Check result has details
        REQUIRE(result.evaluationDetails.has_value());
        CHECK(result.evaluationDetails->flagKey == "json-config-flag");
        REQUIRE(result.evaluationDetails->flagEvaluationCode.has_value());
        CHECK(*result.evaluationDetails->flagEvaluationCode == FlagEvaluationCode::MATCH);
        REQUIRE(result.evaluationDetails->variationKey.has_value());

        // Check variation is valid JSON
        CHECK(result.variation.is_object());
    }

    SECTION("Non-existent flag returns default JSON") {
        json defaultJson = {{"fallback", true}};

        auto result = client.getJsonAssignmentDetails("non-existent-json-flag", "test-subject",
                                                      Attributes(), defaultJson);

        CHECK(result.variation == defaultJson);

        REQUIRE(result.evaluationDetails.has_value());
        REQUIRE(result.evaluationDetails->flagEvaluationCode.has_value());
        CHECK(*result.evaluationDetails->flagEvaluationCode ==
              FlagEvaluationCode::FLAG_UNRECOGNIZED_OR_DISABLED);
    }
}

TEST_CASE("Assignment Details - getSerializedJsonAssignmentDetails", "[assignment-details]") {
    ConfigurationStore configStore;
    ConfigResponse ufc = loadFlagsConfiguration("test/data/ufc/flags-v1.json");
    configStore.setConfiguration(Configuration(ufc));

    auto mockAppLogger = std::make_shared<MockApplicationLogger>();
    EppoClient client(configStore, nullptr, nullptr, mockAppLogger);

    SECTION("Successful stringified JSON assignment with details") {
        std::string defaultValue = R"({"default":"value"})";

        auto result = client.getSerializedJsonAssignmentDetails(
            "json-config-flag", "test-subject-1", Attributes(), defaultValue);

        // Check result has details
        REQUIRE(result.evaluationDetails.has_value());
        CHECK(result.evaluationDetails->flagKey == "json-config-flag");
        REQUIRE(result.evaluationDetails->flagEvaluationCode.has_value());
        CHECK(*result.evaluationDetails->flagEvaluationCode == FlagEvaluationCode::MATCH);

        // Check variation is a valid JSON string
        CHECK(result.variation.find("{") != std::string::npos);
        CHECK(result.variation.find("}") != std::string::npos);

        // Verify it can be parsed as JSON
        json parsed = json::parse(result.variation);
        CHECK(parsed.is_object());
    }

    SECTION("Type mismatch returns default string") {
        // Try to get JSON from a boolean flag
        Attributes attrs;
        attrs["should_disable_feature"] = false;

        auto result = client.getSerializedJsonAssignmentDetails(
            "boolean-false-assignment", "test-subject", attrs, R"({"fallback":"data"})");

        CHECK(result.variation == R"({"fallback":"data"})");

        REQUIRE(result.evaluationDetails.has_value());
        REQUIRE(result.evaluationDetails->flagEvaluationCode.has_value());
        CHECK(*result.evaluationDetails->flagEvaluationCode == FlagEvaluationCode::TYPE_MISMATCH);
    }
}

TEST_CASE("Assignment Details - Evaluation details timestamp", "[assignment-details]") {
    ConfigurationStore configStore;
    ConfigResponse ufc = loadFlagsConfiguration("test/data/ufc/flags-v1.json");
    configStore.setConfiguration(Configuration(ufc));

    EppoClient client(configStore, nullptr, nullptr, nullptr);

    SECTION("Evaluation details contain valid timestamp") {
        auto result = client.getBooleanAssignmentDetails("boolean-false-assignment", "test-subject",
                                                         Attributes(), false);

        REQUIRE(result.evaluationDetails.has_value());
        CHECK_FALSE(result.evaluationDetails->timestamp.empty());

        // Verify timestamp format (RFC3339-like: YYYY-MM-DDTHH:MM:SS.sssZ)
        CHECK(result.evaluationDetails->timestamp.find("T") != std::string::npos);
        CHECK(result.evaluationDetails->timestamp.find("Z") != std::string::npos);
    }
}

TEST_CASE("Assignment Details - Subject attributes preserved", "[assignment-details]") {
    ConfigurationStore configStore;
    ConfigResponse ufc = loadFlagsConfiguration("test/data/ufc/flags-v1.json");
    configStore.setConfiguration(Configuration(ufc));

    EppoClient client(configStore, nullptr, nullptr, nullptr);

    SECTION("Subject attributes are included in evaluation details") {
        Attributes attrs;
        attrs["age"] = 25.0;
        attrs["country"] = std::string("USA");
        attrs["premium"] = true;

        auto result =
            client.getStringAssignmentDetails("kill-switch", "test-subject", attrs, "default");

        REQUIRE(result.evaluationDetails.has_value());
        CHECK(result.evaluationDetails->subjectAttributes.size() == 3);

        // Check attributes are preserved
        auto it = result.evaluationDetails->subjectAttributes.find("age");
        REQUIRE(it != result.evaluationDetails->subjectAttributes.end());

        it = result.evaluationDetails->subjectAttributes.find("country");
        REQUIRE(it != result.evaluationDetails->subjectAttributes.end());

        it = result.evaluationDetails->subjectAttributes.find("premium");
        REQUIRE(it != result.evaluationDetails->subjectAttributes.end());
    }
}

TEST_CASE("Assignment Details - Error handling behavior", "[assignment-details]") {
    ConfigurationStore configStore;
    Configuration emptyConfig(ConfigResponse{});
    configStore.setConfiguration(emptyConfig);

    auto mockLogger = std::make_shared<MockApplicationLogger>();
    EppoClient client(configStore, nullptr, nullptr, mockLogger);

    SECTION("Returns default with error details when flag key is empty") {
        auto result = client.getBooleanAssignmentDetails("",  // Empty flag key to trigger error
                                                         "test-subject", Attributes(), true);

        CHECK(result.variation == true);  // Returns default

        REQUIRE(result.evaluationDetails.has_value());
        REQUIRE(result.evaluationDetails->flagEvaluationCode.has_value());
        CHECK(*result.evaluationDetails->flagEvaluationCode ==
              FlagEvaluationCode::ASSIGNMENT_ERROR);

        // Should have logged error
        CHECK_FALSE(mockLogger->errorMessages.empty());
    }
}

TEST_CASE("Assignment Details - Variation value in details", "[assignment-details]") {
    ConfigurationStore configStore;
    ConfigResponse ufc = loadFlagsConfiguration("test/data/ufc/flags-v1.json");
    configStore.setConfiguration(Configuration(ufc));

    EppoClient client(configStore, nullptr, nullptr, nullptr);

    SECTION("Boolean variation value is present in details") {
        Attributes attrs;
        attrs["should_disable_feature"] = false;

        auto result = client.getBooleanAssignmentDetails("boolean-false-assignment", "test-subject",
                                                         attrs, false);

        REQUIRE(result.evaluationDetails.has_value());
        REQUIRE(result.evaluationDetails->variationValue.has_value());

        // Check the variation value matches the returned variation
        bool valueInDetails = std::get<bool>(*result.evaluationDetails->variationValue);
        CHECK(valueInDetails == result.variation);
    }

    SECTION("Numeric variation value is present in details") {
        auto result =
            client.getNumericAssignmentDetails("numeric_flag", "test-subject", Attributes(), 0.0);

        REQUIRE(result.evaluationDetails.has_value());
        REQUIRE(result.evaluationDetails->variationValue.has_value());

        double valueInDetails = std::get<double>(*result.evaluationDetails->variationValue);
        CHECK(valueInDetails == result.variation);
    }

    SECTION("String variation value is present in details") {
        auto result = client.getStringAssignmentDetails("empty_string_flag", "test-subject",
                                                        Attributes(), "default");

        REQUIRE(result.evaluationDetails.has_value());
        REQUIRE(result.evaluationDetails->variationValue.has_value());

        std::string valueInDetails =
            std::get<std::string>(*result.evaluationDetails->variationValue);
        CHECK(valueInDetails == result.variation);
    }
}

TEST_CASE("Assignment Details - Multiple flags in sequence", "[assignment-details]") {
    ConfigurationStore configStore;
    ConfigResponse ufc = loadFlagsConfiguration("test/data/ufc/flags-v1.json");
    configStore.setConfiguration(Configuration(ufc));

    EppoClient client(configStore, nullptr, nullptr, nullptr);

    SECTION("Can get details for multiple flags in sequence") {
        // Get boolean flag details
        auto result1 = client.getBooleanAssignmentDetails("boolean-false-assignment", "user1",
                                                          Attributes(), false);
        REQUIRE(result1.evaluationDetails.has_value());
        CHECK(result1.evaluationDetails->flagKey == "boolean-false-assignment");

        // Get string flag details
        auto result2 =
            client.getStringAssignmentDetails("kill-switch", "user2", Attributes(), "default");
        REQUIRE(result2.evaluationDetails.has_value());
        CHECK(result2.evaluationDetails->flagKey == "kill-switch");

        // Get numeric flag details
        auto result3 =
            client.getNumericAssignmentDetails("numeric_flag", "user3", Attributes(), 0.0);
        REQUIRE(result3.evaluationDetails.has_value());
        CHECK(result3.evaluationDetails->flagKey == "numeric_flag");

        // Verify each has unique details
        CHECK(result1.evaluationDetails->flagKey != result2.evaluationDetails->flagKey);
        CHECK(result2.evaluationDetails->flagKey != result3.evaluationDetails->flagKey);
    }
}

TEST_CASE("Assignment Details - Empty flag key handling", "[assignment-details]") {
    ConfigurationStore configStore;
    ConfigResponse ufc = loadFlagsConfiguration("test/data/ufc/flags-v1.json");
    configStore.setConfiguration(Configuration(ufc));

    auto mockLogger = std::make_shared<MockApplicationLogger>();
    EppoClient client(configStore, nullptr, nullptr, mockLogger);

    SECTION("Empty flag key in graceful mode returns default with details") {
        auto result = client.getBooleanAssignmentDetails("", "test-subject", Attributes(), true);

        CHECK(result.variation == true);

        REQUIRE(result.evaluationDetails.has_value());
        CHECK(result.evaluationDetails->flagKey.empty());
        REQUIRE(result.evaluationDetails->flagEvaluationCode.has_value());
        CHECK(*result.evaluationDetails->flagEvaluationCode ==
              FlagEvaluationCode::ASSIGNMENT_ERROR);
    }
}

TEST_CASE("Assignment Details - Assignment logging still works", "[assignment-details]") {
    ConfigurationStore configStore;
    ConfigResponse ufc = loadFlagsConfiguration("test/data/ufc/flags-v1.json");
    configStore.setConfiguration(Configuration(ufc));

    auto mockAssignmentLogger = std::make_shared<MockAssignmentLogger>();
    EppoClient client(configStore, mockAssignmentLogger, nullptr, nullptr);

    SECTION("Assignment events are still logged with details methods") {
        Attributes attrs;
        attrs["should_disable_feature"] = false;

        mockAssignmentLogger->clear();

        auto result = client.getBooleanAssignmentDetails("boolean-false-assignment", "test-subject",
                                                         attrs, false);

        // Check that assignment was logged
        CHECK(mockAssignmentLogger->loggedEvents.size() == 1);

        if (!mockAssignmentLogger->loggedEvents.empty()) {
            const auto& event = mockAssignmentLogger->loggedEvents[0];
            CHECK(event.featureFlag == "boolean-false-assignment");
            CHECK(event.subject == "test-subject");
        }
    }
}

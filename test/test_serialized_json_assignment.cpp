#include <catch_amalgamated.hpp>
#include "../src/client.hpp"
#include "../src/config_response.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <vector>

using namespace eppoclient;
using json = nlohmann::json;

namespace {
    // Mock logger to capture log messages for testing
    class MockApplicationLogger : public ApplicationLogger {
    public:
        std::vector<std::string> debugMessages;
        std::vector<std::string> infoMessages;
        std::vector<std::string> warnMessages;
        std::vector<std::string> errorMessages;

        void debug(const std::string& message) override {
            debugMessages.push_back(message);
        }

        void info(const std::string& message) override {
            infoMessages.push_back(message);
        }

        void warn(const std::string& message) override {
            warnMessages.push_back(message);
        }

        void error(const std::string& message) override {
            errorMessages.push_back(message);
        }

        void clear() {
            debugMessages.clear();
            infoMessages.clear();
            warnMessages.clear();
            errorMessages.clear();
        }
    };
    // Helper function to load flags configuration from JSON file
    ConfigResponse loadFlagsConfiguration(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open flags configuration file: " + filepath);
        }

        json j;
        file >> j;

        ConfigResponse response = j;
        return response;
    }

    // Helper function to parse attributes from JSON
    Attributes parseAttributes(const json& attrJson) {
        Attributes attributes;

        for (auto it = attrJson.begin(); it != attrJson.end(); ++it) {
            std::string key = it.key();
            const auto& value = it.value();

            if (value.is_string()) {
                attributes[key] = value.get<std::string>();
            } else if (value.is_number_integer()) {
                attributes[key] = value.get<int64_t>();
            } else if (value.is_number_float()) {
                attributes[key] = value.get<double>();
            } else if (value.is_boolean()) {
                attributes[key] = value.get<bool>();
            } else if (value.is_null()) {
                continue;
            }
        }

        return attributes;
    }
} // anonymous namespace

TEST_CASE("getSerializedJSONAssignment - Basic functionality", "[serialized-json]") {
    // Load flags configuration
    std::string flagsPath = "test/data/ufc/flags-v1.json";
    ConfigResponse configResponse = loadFlagsConfiguration(flagsPath);

    // Create configuration
    Configuration config(configResponse);
    config.precompute();

    // Create client
    auto configStore = std::make_shared<ConfigurationStore>();
    configStore->setConfiguration(config);
    auto client = std::make_shared<EppoClient>(configStore, nullptr, nullptr);

    SECTION("Returns stringified JSON for valid assignment") {
        // Test with alice who should get {"integer": 1, "string": "one", "float": 1.0}
        Attributes aliceAttrs;
        aliceAttrs["email"] = std::string("alice@mycompany.com");
        aliceAttrs["country"] = std::string("US");

        std::string defaultValue = R"({"foo":"bar"})";
        std::string result = client->getSerializedJSONAssignment(
            "json-config-flag",
            "alice",
            aliceAttrs,
            defaultValue
        );

        // Verify result is not the default
        REQUIRE(result != defaultValue);

        // Verify result is valid JSON
        json parsedResult;
        REQUIRE_NOTHROW(parsedResult = json::parse(result));

        // Verify the structure
        REQUIRE(parsedResult.is_object());
        REQUIRE(parsedResult.contains("integer"));
        REQUIRE(parsedResult.contains("string"));
        REQUIRE(parsedResult.contains("float"));

        // Verify the values
        CHECK(parsedResult["integer"].get<int>() == 1);
        CHECK(parsedResult["string"].get<std::string>() == "one");
        CHECK(parsedResult["float"].get<double>() == 1.0);
    }

    SECTION("Returns different assignments for different subjects") {
        Attributes bobAttrs;
        bobAttrs["email"] = std::string("bob@example.com");
        bobAttrs["country"] = std::string("Canada");

        std::string defaultValue = R"({"foo":"bar"})";
        std::string result = client->getSerializedJSONAssignment(
            "json-config-flag",
            "bob",
            bobAttrs,
            defaultValue
        );

        // Verify result is valid JSON
        json parsedResult = json::parse(result);

        // Bob should get {"integer": 2, "string": "two", "float": 2.0}
        REQUIRE(parsedResult.is_object());
        CHECK(parsedResult["integer"].get<int>() == 2);
        CHECK(parsedResult["string"].get<std::string>() == "two");
        CHECK(parsedResult["float"].get<double>() == 2.0);
    }

    SECTION("Handles empty JSON object") {
        // Diana with Force Empty should get {}
        Attributes dianaAttrs;
        dianaAttrs["Force Empty"] = true;

        std::string defaultValue = R"({"foo":"bar"})";
        std::string result = client->getSerializedJSONAssignment(
            "json-config-flag",
            "diana",
            dianaAttrs,
            defaultValue
        );

        // Verify result is valid JSON
        json parsedResult = json::parse(result);

        // Should be empty object
        REQUIRE(parsedResult.is_object());
        CHECK(parsedResult.empty());
        CHECK(result == "{}");
    }
}

TEST_CASE("getSerializedJSONAssignment - Default value behavior", "[serialized-json]") {
    // Load flags configuration
    std::string flagsPath = "test/data/ufc/flags-v1.json";
    ConfigResponse configResponse = loadFlagsConfiguration(flagsPath);

    // Create configuration
    Configuration config(configResponse);
    config.precompute();

    // Create client
    auto configStore = std::make_shared<ConfigurationStore>();
    configStore->setConfiguration(config);
    auto client = std::make_shared<EppoClient>(configStore, nullptr, nullptr);

    SECTION("Returns default value for non-existent flag") {
        Attributes attrs;
        attrs["email"] = std::string("test@example.com");

        std::string defaultValue = R"({"default":"value"})";
        std::string result = client->getSerializedJSONAssignment(
            "non-existent-flag",
            "test-subject",
            attrs,
            defaultValue
        );

        CHECK(result == defaultValue);
    }

    SECTION("Returns default value for empty subject key") {
        Attributes attrs;
        std::string defaultValue = R"({"default":"value"})";

        std::string result = client->getSerializedJSONAssignment(
            "json-config-flag",
            "",
            attrs,
            defaultValue
        );

        CHECK(result == defaultValue);
    }

    SECTION("Returns default value for empty flag key") {
        Attributes attrs;
        std::string defaultValue = R"({"default":"value"})";

        std::string result = client->getSerializedJSONAssignment(
            "",
            "test-subject",
            attrs,
            defaultValue
        );

        CHECK(result == defaultValue);
    }
}

TEST_CASE("getSerializedJSONAssignment - Type mismatch with application logger", "[serialized-json]") {
    // Load flags configuration
    std::string flagsPath = "test/data/ufc/flags-v1.json";
    ConfigResponse configResponse = loadFlagsConfiguration(flagsPath);

    // Create configuration
    Configuration config(configResponse);
    config.precompute();

    // Create mock logger to capture error messages
    auto mockLogger = std::make_shared<MockApplicationLogger>();

    // Create client with mock logger
    auto configStore = std::make_shared<ConfigurationStore>();
    configStore->setConfiguration(config);
    auto client = std::make_shared<EppoClient>(configStore, nullptr, nullptr, mockLogger);

    SECTION("Calling getSerializedJSONAssignment on an INTEGER flag logs type mismatch error") {
        Attributes aliceAttrs;
        aliceAttrs["email"] = std::string("alice@mycompany.com");
        aliceAttrs["country"] = std::string("US");

        std::string defaultValue = R"({"default":"value"})";

        // Call getSerializedJSONAssignment on an integer flag (integer-flag returns int values)
        std::string result = client->getSerializedJSONAssignment(
            "integer-flag",
            "alice",
            aliceAttrs,
            defaultValue
        );

        // Should return the default value because of type mismatch
        CHECK(result == defaultValue);

        // Should have logged a warning from getAssignment and an error from the catch block
        REQUIRE(mockLogger->warnMessages.size() == 1);
        REQUIRE(mockLogger->errorMessages.size() == 1);

        // Verify the warning message (from getAssignment's type verification)
        std::string warnMsg = mockLogger->warnMessages[0];
        INFO("Warning message: " << warnMsg);
        CHECK(warnMsg.find("Failed to verify flag type") != std::string::npos);
        CHECK(warnMsg.find("unexpected variation type") != std::string::npos);

        // Verify the error message format (from getSerializedJSONAssignment's catch block)
        std::string errorMsg = mockLogger->errorMessages[0];
        INFO("Error message: " << errorMsg);
        CHECK(errorMsg.find("Error in getSerializedJSONAssignment") != std::string::npos);
        CHECK(errorMsg.find("unexpected variation type") != std::string::npos);

        // The message also includes expected type (4 = JSON) and actual type (1 = INTEGER)
        CHECK(errorMsg.find("expected: 4") != std::string::npos);
        CHECK(errorMsg.find("actual: 1") != std::string::npos);
    }

    SECTION("Calling getSerializedJSONAssignment on a STRING flag logs type mismatch error") {
        mockLogger->clear();

        Attributes attrs;
        attrs["email"] = std::string("alice@example.com");

        std::string defaultValue = R"({"default":"value"})";

        // There should be a string flag in the test data - let's use "empty_string_flag"
        std::string result = client->getSerializedJSONAssignment(
            "empty_string_flag",
            "alice",
            attrs,
            defaultValue
        );

        // Should return the default value because of type mismatch
        CHECK(result == defaultValue);

        // Should have logged a warning and an error
        REQUIRE(mockLogger->warnMessages.size() == 1);
        REQUIRE(mockLogger->errorMessages.size() == 1);

        // Verify the error message format
        std::string errorMsg = mockLogger->errorMessages[0];
        INFO("Error message: " << errorMsg);

        CHECK(errorMsg.find("Error in getSerializedJSONAssignment") != std::string::npos);
        CHECK(errorMsg.find("unexpected variation type") != std::string::npos);
        // expected: 4 (JSON), actual: 0 (STRING)
        CHECK(errorMsg.find("expected: 4") != std::string::npos);
        CHECK(errorMsg.find("actual: 0") != std::string::npos);
    }

    SECTION("Calling getSerializedJSONAssignment on a BOOLEAN flag logs type mismatch error") {
        mockLogger->clear();

        Attributes attrs;

        std::string defaultValue = R"({"default":"value"})";

        // Use a boolean flag
        std::string result = client->getSerializedJSONAssignment(
            "kill-switch",
            "test-subject",
            attrs,
            defaultValue
        );

        // Should return the default value because of type mismatch
        CHECK(result == defaultValue);

        // Should have logged a warning and an error
        REQUIRE(mockLogger->warnMessages.size() == 1);
        REQUIRE(mockLogger->errorMessages.size() == 1);

        // Verify the error message format
        std::string errorMsg = mockLogger->errorMessages[0];
        INFO("Error message: " << errorMsg);

        CHECK(errorMsg.find("Error in getSerializedJSONAssignment") != std::string::npos);
        CHECK(errorMsg.find("unexpected variation type") != std::string::npos);
        // expected: 4 (JSON), actual: 3 (BOOLEAN)
        CHECK(errorMsg.find("expected: 4") != std::string::npos);
        CHECK(errorMsg.find("actual: 3") != std::string::npos);
    }

    SECTION("Calling getSerializedJSONAssignment on a NUMERIC flag logs type mismatch error") {
        mockLogger->clear();

        Attributes attrs;

        std::string defaultValue = R"({"default":"value"})";

        // Use a numeric flag
        std::string result = client->getSerializedJSONAssignment(
            "numeric_flag",
            "test-subject",
            attrs,
            defaultValue
        );

        // Should return the default value because of type mismatch
        CHECK(result == defaultValue);

        // Should have logged a warning and an error
        REQUIRE(mockLogger->warnMessages.size() == 1);
        REQUIRE(mockLogger->errorMessages.size() == 1);

        // Verify the error message format
        std::string errorMsg = mockLogger->errorMessages[0];
        INFO("Error message: " << errorMsg);

        CHECK(errorMsg.find("Error in getSerializedJSONAssignment") != std::string::npos);
        CHECK(errorMsg.find("unexpected variation type") != std::string::npos);
        // expected: 4 (JSON), actual: 2 (NUMERIC)
        CHECK(errorMsg.find("expected: 4") != std::string::npos);
        CHECK(errorMsg.find("actual: 2") != std::string::npos);
    }

    SECTION("Calling getSerializedJSONAssignment on a JSON flag does NOT log errors") {
        mockLogger->clear();

        Attributes aliceAttrs;
        aliceAttrs["email"] = std::string("alice@mycompany.com");
        aliceAttrs["country"] = std::string("US");

        std::string defaultValue = R"({"default":"value"})";

        // Call on the correct type - JSON flag
        std::string result = client->getSerializedJSONAssignment(
            "json-config-flag",
            "alice",
            aliceAttrs,
            defaultValue
        );

        // Should NOT return the default value
        CHECK(result != defaultValue);

        // Should NOT have logged any errors
        CHECK(mockLogger->errorMessages.empty());

        // Result should be valid JSON
        json parsed;
        REQUIRE_NOTHROW(parsed = json::parse(result));
    }
}

TEST_CASE("getSerializedJSONAssignment - Complex JSON structures", "[serialized-json]") {
    // Load flags configuration
    std::string flagsPath = "test/data/ufc/flags-v1.json";
    ConfigResponse configResponse = loadFlagsConfiguration(flagsPath);

    // Create configuration
    Configuration config(configResponse);
    config.precompute();

    // Create client
    auto configStore = std::make_shared<ConfigurationStore>();
    configStore->setConfiguration(config);
    auto client = std::make_shared<EppoClient>(configStore, nullptr, nullptr);

    SECTION("Handles nested JSON structures") {
        Attributes aliceAttrs;
        aliceAttrs["email"] = std::string("alice@mycompany.com");
        aliceAttrs["country"] = std::string("US");

        std::string defaultValue = R"({"nested":{"deep":{"value":"default"}}})";
        std::string result = client->getSerializedJSONAssignment(
            "json-config-flag",
            "alice",
            aliceAttrs,
            defaultValue
        );

        // Verify result is valid JSON
        json parsedResult;
        REQUIRE_NOTHROW(parsedResult = json::parse(result));

        // Should have the expected structure from the flag
        REQUIRE(parsedResult.is_object());
        REQUIRE(parsedResult.contains("integer"));
    }

    SECTION("Default with special characters is preserved") {
        Attributes attrs;
        std::string defaultValue = R"({"special":"chars: \n\t\r\"\\","unicode":"测试"})";

        std::string result = client->getSerializedJSONAssignment(
            "non-existent-flag",
            "test-subject",
            attrs,
            defaultValue
        );

        CHECK(result == defaultValue);

        // Should be parseable
        json parsed;
        REQUIRE_NOTHROW(parsed = json::parse(result));
    }

    SECTION("Handles JSON arrays in values") {
        // The actual assignment might have different structure,
        // but we test that arrays in defaults work
        Attributes attrs;
        std::string defaultValue = R"({"array":[1,2,3],"nested":{"array":["a","b","c"]}})";

        std::string result = client->getSerializedJSONAssignment(
            "non-existent-flag",
            "test-subject",
            attrs,
            defaultValue
        );

        CHECK(result == defaultValue);

        // Should be parseable
        json parsed = json::parse(result);
        REQUIRE(parsed.contains("array"));
        REQUIRE(parsed["array"].is_array());
        CHECK(parsed["array"].size() == 3);
    }
}

TEST_CASE("getSerializedJSONAssignment - JSON formatting", "[serialized-json]") {
    // Load flags configuration
    std::string flagsPath = "test/data/ufc/flags-v1.json";
    ConfigResponse configResponse = loadFlagsConfiguration(flagsPath);

    // Create configuration
    Configuration config(configResponse);
    config.precompute();

    // Create client
    auto configStore = std::make_shared<ConfigurationStore>();
    configStore->setConfiguration(config);
    auto client = std::make_shared<EppoClient>(configStore, nullptr, nullptr);

    SECTION("Returns compact JSON (no pretty printing)") {
        Attributes aliceAttrs;
        aliceAttrs["email"] = std::string("alice@mycompany.com");
        aliceAttrs["country"] = std::string("US");

        std::string defaultValue = R"({"foo":"bar"})";
        std::string result = client->getSerializedJSONAssignment(
            "json-config-flag",
            "alice",
            aliceAttrs,
            defaultValue
        );

        // Result should not contain newlines (compact format)
        CHECK(result.find('\n') == std::string::npos);
        CHECK(result.find("  ") == std::string::npos); // No double spaces
    }

    SECTION("Preserves JSON key order (as per nlohmann::json)") {
        Attributes aliceAttrs;
        aliceAttrs["email"] = std::string("alice@mycompany.com");
        aliceAttrs["country"] = std::string("US");

        std::string defaultValue = R"({"foo":"bar"})";
        std::string result = client->getSerializedJSONAssignment(
            "json-config-flag",
            "alice",
            aliceAttrs,
            defaultValue
        );

        // Parse and verify structure
        json parsed = json::parse(result);
        REQUIRE(parsed.is_object());

        // The exact order may vary, but all keys should be present
        std::vector<std::string> keys;
        for (auto it = parsed.begin(); it != parsed.end(); ++it) {
            keys.push_back(it.key());
        }
        CHECK(keys.size() == 3);
    }
}

TEST_CASE("getSerializedJSONAssignment - All test case subjects", "[serialized-json][ufc]") {
    // Load flags configuration
    std::string flagsPath = "test/data/ufc/flags-v1.json";
    ConfigResponse configResponse = loadFlagsConfiguration(flagsPath);

    // Create configuration
    Configuration config(configResponse);
    config.precompute();

    // Create client
    auto configStore = std::make_shared<ConfigurationStore>();
    configStore->setConfiguration(config);
    auto client = std::make_shared<EppoClient>(configStore, nullptr, nullptr);

    // Load test case
    std::ifstream file("test/data/ufc/tests/test-json-config-flag.json");
    REQUIRE(file.is_open());

    json testCase;
    file >> testCase;

    std::string flagKey = testCase["flag"].get<std::string>();
    json defaultValueJson = testCase["defaultValue"];
    std::string defaultValue = defaultValueJson.dump();

    // Test each subject
    for (const auto& subjectJson : testCase["subjects"]) {
        std::string subjectKey = subjectJson["subjectKey"].get<std::string>();
        Attributes attrs = parseAttributes(subjectJson["subjectAttributes"]);
        json expectedAssignment = subjectJson["assignment"];

        DYNAMIC_SECTION("Subject: " << subjectKey) {
            // Get serialized assignment
            std::string result = client->getSerializedJSONAssignment(
                flagKey,
                subjectKey,
                attrs,
                defaultValue
            );

            // Parse result
            json parsedResult = json::parse(result);

            // Compare with expected
            INFO("Expected: " << expectedAssignment.dump());
            INFO("Actual: " << parsedResult.dump());
            CHECK(parsedResult == expectedAssignment);
        }
    }
}

#include <catch_amalgamated.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include "../src/client.hpp"
#include "../src/config_response.hpp"

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
}  // anonymous namespace

TEST_CASE("Error handling - returns default values and logs errors", "[error-handling]") {
    // Create a configuration store with empty config (no flags)
    auto configStore = std::make_shared<ConfigurationStore>();
    Configuration emptyConfig(ConfigResponse{});
    configStore->setConfiguration(emptyConfig);

    auto mockLogger = std::make_shared<MockApplicationLogger>();

    // Create client
    EppoClient client(configStore, nullptr, nullptr, mockLogger);

    Attributes attrs;
    attrs["test"] = std::string("value");

    SECTION("getBooleanAssignment returns default value on empty subject key error") {
        bool result = client.getBooleanAssignment("test-flag", "", attrs, true);

        // Should return default value
        CHECK(result == true);

        // Should have logged error
        REQUIRE(mockLogger->errorMessages.size() >= 1);
        bool foundError = false;
        for (const auto& msg : mockLogger->errorMessages) {
            if (msg.find("No subject key provided") != std::string::npos) {
                foundError = true;
                break;
            }
        }
        CHECK(foundError);
    }

    SECTION("getNumericAssignment returns default value on empty subject key error") {
        mockLogger->clear();
        double result = client.getNumericAssignment("test-flag", "", attrs, 42.5);

        // Should return default value
        CHECK(result == 42.5);

        // Should have logged error
        REQUIRE(mockLogger->errorMessages.size() >= 1);
        bool foundError = false;
        for (const auto& msg : mockLogger->errorMessages) {
            if (msg.find("No subject key provided") != std::string::npos) {
                foundError = true;
                break;
            }
        }
        CHECK(foundError);
    }

    SECTION("getIntegerAssignment returns default value on empty flag key error") {
        mockLogger->clear();
        int64_t result = client.getIntegerAssignment("", "test-subject", attrs, 123);

        // Should return default value
        CHECK(result == 123);

        // Should have logged error
        REQUIRE(mockLogger->errorMessages.size() >= 1);
        bool foundError = false;
        for (const auto& msg : mockLogger->errorMessages) {
            if (msg.find("No flag key provided") != std::string::npos) {
                foundError = true;
                break;
            }
        }
        CHECK(foundError);
    }

    SECTION("getStringAssignment returns default value on empty flag key error") {
        mockLogger->clear();
        std::string result = client.getStringAssignment("", "test-subject", attrs, "default");

        // Should return default value
        CHECK(result == "default");

        // Should have logged error
        REQUIRE(mockLogger->errorMessages.size() >= 1);
        bool foundError = false;
        for (const auto& msg : mockLogger->errorMessages) {
            if (msg.find("No flag key provided") != std::string::npos) {
                foundError = true;
                break;
            }
        }
        CHECK(foundError);
    }

    SECTION("getJSONAssignment returns default value on empty subject key error") {
        mockLogger->clear();
        json defaultJson = {{"key", "value"}};
        json result = client.getJSONAssignment("test-flag", "", attrs, defaultJson);

        // Should return default value
        CHECK(result == defaultJson);

        // Should have logged error
        REQUIRE(mockLogger->errorMessages.size() >= 1);
        bool foundError = false;
        for (const auto& msg : mockLogger->errorMessages) {
            if (msg.find("No subject key provided") != std::string::npos) {
                foundError = true;
                break;
            }
        }
        CHECK(foundError);
    }

    SECTION("getSerializedJSONAssignment returns default value on empty subject key error") {
        mockLogger->clear();
        std::string defaultValue = R"({"default":"value"})";
        std::string result =
            client.getSerializedJSONAssignment("test-flag", "", attrs, defaultValue);

        // Should return default value
        CHECK(result == defaultValue);

        // Should have logged error
        REQUIRE(mockLogger->errorMessages.size() >= 1);
        bool foundError = false;
        for (const auto& msg : mockLogger->errorMessages) {
            if (msg.find("No subject key provided") != std::string::npos) {
                foundError = true;
                break;
            }
        }
        CHECK(foundError);
    }
}

TEST_CASE("Error handling - Empty subject key errors", "[error-handling]") {
    // Create a working configuration store with empty config
    auto configStore = std::make_shared<ConfigurationStore>();
    Configuration emptyConfig(ConfigResponse{});
    configStore->setConfiguration(emptyConfig);

    auto mockLogger = std::make_shared<MockApplicationLogger>();
    EppoClient client(configStore, nullptr, nullptr, mockLogger);

    Attributes attrs;
    attrs["test"] = std::string("value");

    SECTION("Empty subject key returns default and logs error") {
        bool result = client.getBooleanAssignment("test-flag", "", attrs, true);
        CHECK(result == true);

        // Should have logged error
        REQUIRE(mockLogger->errorMessages.size() >= 1);
        bool foundSubjectKeyError = false;
        for (const auto& msg : mockLogger->errorMessages) {
            if (msg.find("No subject key provided") != std::string::npos) {
                foundSubjectKeyError = true;
                break;
            }
        }
        CHECK(foundSubjectKeyError);
    }
}

TEST_CASE("Error handling - Empty flag key errors", "[error-handling]") {
    // Create a working configuration store with empty config
    auto configStore = std::make_shared<ConfigurationStore>();
    Configuration emptyConfig(ConfigResponse{});
    configStore->setConfiguration(emptyConfig);

    auto mockLogger = std::make_shared<MockApplicationLogger>();
    EppoClient client(configStore, nullptr, nullptr, mockLogger);

    Attributes attrs;
    attrs["test"] = std::string("value");

    SECTION("Empty flag key returns default and logs error") {
        bool result = client.getBooleanAssignment("", "test-subject", attrs, false);
        CHECK(result == false);

        // Should have logged error
        REQUIRE(mockLogger->errorMessages.size() >= 1);
        bool foundFlagKeyError = false;
        for (const auto& msg : mockLogger->errorMessages) {
            if (msg.find("No flag key provided") != std::string::npos) {
                foundFlagKeyError = true;
                break;
            }
        }
        CHECK(foundFlagKeyError);
    }
}

TEST_CASE("Error handling - Missing flag configuration", "[error-handling]") {
    // Create a configuration store with empty config
    auto configStore = std::make_shared<ConfigurationStore>();
    Configuration emptyConfig(ConfigResponse{});
    configStore->setConfiguration(emptyConfig);

    auto mockLogger = std::make_shared<MockApplicationLogger>();
    EppoClient client(configStore, nullptr, nullptr, mockLogger);

    Attributes attrs;
    attrs["test"] = std::string("value");

    SECTION("Missing flag returns default and logs info") {
        bool result = client.getBooleanAssignment("nonexistent-flag", "test-subject", attrs, true);
        CHECK(result == true);

        // Should have logged info about missing flag
        REQUIRE(mockLogger->infoMessages.size() >= 1);
        bool foundMissingFlag = false;
        for (const auto& msg : mockLogger->infoMessages) {
            if (msg.find("Failed to get flag configuration") != std::string::npos) {
                foundMissingFlag = true;
                break;
            }
        }
        CHECK(foundMissingFlag);
    }
}

#include <catch_amalgamated.hpp>
#include "../src/client.hpp"
#include "../src/config_response.hpp"
#include <nlohmann/json.hpp>
#include <memory>

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
} // anonymous namespace

TEST_CASE("Graceful failure mode - Default behavior (graceful mode enabled)", "[graceful-failure]") {
    // Create a configuration store with empty config (no flags)
    auto configStore = std::make_shared<ConfigurationStore>();
    Configuration emptyConfig(ConfigResponse{});
    configStore->setConfiguration(emptyConfig);

    auto mockLogger = std::make_shared<MockApplicationLogger>();

    // Create client (graceful mode is enabled by default)
    auto client = std::make_shared<EppoClient>(configStore, nullptr, nullptr, mockLogger);

    Attributes attrs;
    attrs["test"] = std::string("value");

    SECTION("getBoolAssignment returns default value on empty subject key error") {
        bool result = client->getBoolAssignment("test-flag", "", attrs, true);

        // Should return default value
        CHECK(result == true);

        // Should have logged error
        REQUIRE(mockLogger->errorMessages.size() >= 1);
        bool foundError = false;
        for (const auto& msg : mockLogger->errorMessages) {
            if (msg.find("Error in getBoolAssignment") != std::string::npos) {
                foundError = true;
                break;
            }
        }
        CHECK(foundError);
    }

    SECTION("getNumericAssignment returns default value on empty subject key error") {
        mockLogger->clear();
        double result = client->getNumericAssignment("test-flag", "", attrs, 42.5);

        // Should return default value
        CHECK(result == 42.5);

        // Should have logged error
        REQUIRE(mockLogger->errorMessages.size() >= 1);
        bool foundError = false;
        for (const auto& msg : mockLogger->errorMessages) {
            if (msg.find("Error in getNumericAssignment") != std::string::npos) {
                foundError = true;
                break;
            }
        }
        CHECK(foundError);
    }

    SECTION("getIntegerAssignment returns default value on empty flag key error") {
        mockLogger->clear();
        int64_t result = client->getIntegerAssignment("", "test-subject", attrs, 123);

        // Should return default value
        CHECK(result == 123);

        // Should have logged error
        REQUIRE(mockLogger->errorMessages.size() >= 1);
        bool foundError = false;
        for (const auto& msg : mockLogger->errorMessages) {
            if (msg.find("Error in getIntegerAssignment") != std::string::npos) {
                foundError = true;
                break;
            }
        }
        CHECK(foundError);
    }

    SECTION("getStringAssignment returns default value on empty flag key error") {
        mockLogger->clear();
        std::string result = client->getStringAssignment("", "test-subject", attrs, "default");

        // Should return default value
        CHECK(result == "default");

        // Should have logged error
        REQUIRE(mockLogger->errorMessages.size() >= 1);
        bool foundError = false;
        for (const auto& msg : mockLogger->errorMessages) {
            if (msg.find("Error in getStringAssignment") != std::string::npos) {
                foundError = true;
                break;
            }
        }
        CHECK(foundError);
    }

    SECTION("getJSONAssignment returns default value on empty subject key error") {
        mockLogger->clear();
        json defaultJson = {{"key", "value"}};
        json result = client->getJSONAssignment("test-flag", "", attrs, defaultJson);

        // Should return default value
        CHECK(result == defaultJson);

        // Should have logged error
        REQUIRE(mockLogger->errorMessages.size() >= 1);
        bool foundError = false;
        for (const auto& msg : mockLogger->errorMessages) {
            if (msg.find("Error in getJSONAssignment") != std::string::npos) {
                foundError = true;
                break;
            }
        }
        CHECK(foundError);
    }

    SECTION("getSerializedJSONAssignment returns default value on empty subject key error") {
        mockLogger->clear();
        std::string defaultValue = R"({"default":"value"})";
        std::string result = client->getSerializedJSONAssignment("test-flag", "", attrs, defaultValue);

        // Should return default value
        CHECK(result == defaultValue);

        // Should have logged error
        REQUIRE(mockLogger->errorMessages.size() >= 1);
        bool foundError = false;
        for (const auto& msg : mockLogger->errorMessages) {
            if (msg.find("Error in getSerializedJSONAssignment") != std::string::npos) {
                foundError = true;
                break;
            }
        }
        CHECK(foundError);
    }
}

TEST_CASE("Graceful failure mode - Non-graceful mode (exceptions rethrown)", "[graceful-failure]") {
    // Create a configuration store with empty config
    auto configStore = std::make_shared<ConfigurationStore>();
    Configuration emptyConfig(ConfigResponse{});
    configStore->setConfiguration(emptyConfig);

    auto mockLogger = std::make_shared<MockApplicationLogger>();

    // Create client and disable graceful failure mode
    auto client = std::make_shared<EppoClient>(configStore, nullptr, nullptr, mockLogger);
    client->setIsGracefulFailureMode(false);

    Attributes attrs;
    attrs["test"] = std::string("value");

    SECTION("getBoolAssignment rethrows exception after logging") {
        REQUIRE_THROWS_AS(
            client->getBoolAssignment("test-flag", "", attrs, true),
            std::invalid_argument
        );

        // Should have logged error before rethrowing
        REQUIRE(mockLogger->errorMessages.size() >= 1);
        bool foundError = false;
        for (const auto& msg : mockLogger->errorMessages) {
            if (msg.find("Error in getBoolAssignment") != std::string::npos) {
                foundError = true;
                break;
            }
        }
        CHECK(foundError);
    }

    SECTION("getNumericAssignment rethrows exception after logging") {
        mockLogger->clear();
        REQUIRE_THROWS_AS(
            client->getNumericAssignment("test-flag", "", attrs, 42.5),
            std::invalid_argument
        );

        // Should have logged error before rethrowing
        REQUIRE(mockLogger->errorMessages.size() >= 1);
        bool foundError = false;
        for (const auto& msg : mockLogger->errorMessages) {
            if (msg.find("Error in getNumericAssignment") != std::string::npos) {
                foundError = true;
                break;
            }
        }
        CHECK(foundError);
    }

    SECTION("getIntegerAssignment rethrows exception after logging") {
        mockLogger->clear();
        REQUIRE_THROWS_AS(
            client->getIntegerAssignment("", "test-subject", attrs, 123),
            std::invalid_argument
        );

        // Should have logged error before rethrowing
        REQUIRE(mockLogger->errorMessages.size() >= 1);
        bool foundError = false;
        for (const auto& msg : mockLogger->errorMessages) {
            if (msg.find("Error in getIntegerAssignment") != std::string::npos) {
                foundError = true;
                break;
            }
        }
        CHECK(foundError);
    }

    SECTION("getStringAssignment rethrows exception after logging") {
        mockLogger->clear();
        REQUIRE_THROWS_AS(
            client->getStringAssignment("", "test-subject", attrs, "default"),
            std::invalid_argument
        );

        // Should have logged error before rethrowing
        REQUIRE(mockLogger->errorMessages.size() >= 1);
        bool foundError = false;
        for (const auto& msg : mockLogger->errorMessages) {
            if (msg.find("Error in getStringAssignment") != std::string::npos) {
                foundError = true;
                break;
            }
        }
        CHECK(foundError);
    }

    SECTION("getJSONAssignment rethrows exception after logging") {
        mockLogger->clear();
        json defaultJson = {{"key", "value"}};
        REQUIRE_THROWS_AS(
            client->getJSONAssignment("test-flag", "", attrs, defaultJson),
            std::invalid_argument
        );

        // Should have logged error before rethrowing
        REQUIRE(mockLogger->errorMessages.size() >= 1);
        bool foundError = false;
        for (const auto& msg : mockLogger->errorMessages) {
            if (msg.find("Error in getJSONAssignment") != std::string::npos) {
                foundError = true;
                break;
            }
        }
        CHECK(foundError);
    }

    SECTION("getSerializedJSONAssignment rethrows exception after logging") {
        mockLogger->clear();
        std::string defaultValue = R"({"default":"value"})";
        REQUIRE_THROWS_AS(
            client->getSerializedJSONAssignment("test-flag", "", attrs, defaultValue),
            std::invalid_argument
        );

        // Should have logged error before rethrowing
        REQUIRE(mockLogger->errorMessages.size() >= 1);
        bool foundError = false;
        for (const auto& msg : mockLogger->errorMessages) {
            if (msg.find("Error in getSerializedJSONAssignment") != std::string::npos) {
                foundError = true;
                break;
            }
        }
        CHECK(foundError);
    }
}

TEST_CASE("Graceful failure mode - Toggling behavior", "[graceful-failure]") {
    // Create a configuration store with empty config
    auto configStore = std::make_shared<ConfigurationStore>();
    Configuration emptyConfig(ConfigResponse{});
    configStore->setConfiguration(emptyConfig);

    auto mockLogger = std::make_shared<MockApplicationLogger>();

    auto client = std::make_shared<EppoClient>(configStore, nullptr, nullptr, mockLogger);

    Attributes attrs;
    attrs["test"] = std::string("value");

    SECTION("Can toggle from graceful to non-graceful mode") {
        // Start in graceful mode (default) - trigger error with empty subject key
        bool result1 = client->getBoolAssignment("test-flag", "", attrs, false);
        CHECK(result1 == false);  // Returns default

        mockLogger->clear();

        // Switch to non-graceful mode
        client->setIsGracefulFailureMode(false);

        // Now it should throw
        REQUIRE_THROWS_AS(
            client->getBoolAssignment("test-flag", "", attrs, false),
            std::invalid_argument
        );
    }

    SECTION("Can toggle from non-graceful to graceful mode") {
        // Switch to non-graceful mode
        client->setIsGracefulFailureMode(false);

        // Should throw with empty subject key
        REQUIRE_THROWS_AS(
            client->getBoolAssignment("test-flag", "", attrs, false),
            std::invalid_argument
        );

        mockLogger->clear();

        // Switch back to graceful mode
        client->setIsGracefulFailureMode(true);

        // Should return default now
        bool result = client->getBoolAssignment("test-flag", "", attrs, true);
        CHECK(result == true);
    }

    SECTION("Can toggle multiple times") {
        // Start graceful (default) - use empty subject key to trigger error
        bool result1 = client->getBoolAssignment("test-flag", "", attrs, true);
        CHECK(result1 == true);

        // Toggle to non-graceful
        client->setIsGracefulFailureMode(false);
        REQUIRE_THROWS(client->getBoolAssignment("test-flag", "", attrs, true));

        // Toggle back to graceful
        client->setIsGracefulFailureMode(true);
        bool result2 = client->getBoolAssignment("test-flag", "", attrs, false);
        CHECK(result2 == false);

        // Toggle to non-graceful again
        client->setIsGracefulFailureMode(false);
        REQUIRE_THROWS(client->getBoolAssignment("test-flag", "", attrs, true));
    }
}

TEST_CASE("Graceful failure mode - Exception type preservation", "[graceful-failure]") {
    // Test that bare throw preserves the original exception type (std::invalid_argument)
    auto configStore = std::make_shared<ConfigurationStore>();
    Configuration emptyConfig(ConfigResponse{});
    configStore->setConfiguration(emptyConfig);

    auto mockLogger = std::make_shared<MockApplicationLogger>();

    auto client = std::make_shared<EppoClient>(configStore, nullptr, nullptr, mockLogger);
    client->setIsGracefulFailureMode(false);

    Attributes attrs;
    attrs["test"] = std::string("value");

    SECTION("Rethrown exception preserves original type (invalid_argument)") {
        try {
            // Empty subject key triggers std::invalid_argument
            client->getBoolAssignment("test-flag", "", attrs, false);
            FAIL("Should have thrown an exception");
        } catch (const std::invalid_argument& e) {
            // Successfully caught the specific exception type
            CHECK(std::string(e.what()).find("no subject key provided") != std::string::npos);
        } catch (const std::exception& e) {
            FAIL("Exception was sliced - caught base type instead of derived type");
        }
    }
}

TEST_CASE("Graceful failure mode - Empty subject key errors", "[graceful-failure]") {
    // Create a working configuration store with empty config
    auto configStore = std::make_shared<ConfigurationStore>();
    Configuration emptyConfig(ConfigResponse{});
    configStore->setConfiguration(emptyConfig);

    auto mockLogger = std::make_shared<MockApplicationLogger>();
    auto client = std::make_shared<EppoClient>(configStore, nullptr, nullptr, mockLogger);

    Attributes attrs;
    attrs["test"] = std::string("value");

    SECTION("Empty subject key in graceful mode returns default") {
        bool result = client->getBoolAssignment("test-flag", "", attrs, true);
        CHECK(result == true);

        // Should have logged error
        REQUIRE(mockLogger->errorMessages.size() >= 1);
        bool foundSubjectKeyError = false;
        for (const auto& msg : mockLogger->errorMessages) {
            if (msg.find("No subject key provided") != std::string::npos ||
                msg.find("no subject key provided") != std::string::npos) {
                foundSubjectKeyError = true;
                break;
            }
        }
        CHECK(foundSubjectKeyError);
    }

    SECTION("Empty subject key in non-graceful mode throws") {
        mockLogger->clear();
        client->setIsGracefulFailureMode(false);

        REQUIRE_THROWS_AS(
            client->getBoolAssignment("test-flag", "", attrs, true),
            std::invalid_argument
        );

        // Should have logged error before throwing
        REQUIRE(mockLogger->errorMessages.size() >= 1);
    }
}

TEST_CASE("Graceful failure mode - Empty flag key errors", "[graceful-failure]") {
    // Create a working configuration store with empty config
    auto configStore = std::make_shared<ConfigurationStore>();
    Configuration emptyConfig(ConfigResponse{});
    configStore->setConfiguration(emptyConfig);

    auto mockLogger = std::make_shared<MockApplicationLogger>();
    auto client = std::make_shared<EppoClient>(configStore, nullptr, nullptr, mockLogger);

    Attributes attrs;
    attrs["test"] = std::string("value");

    SECTION("Empty flag key in graceful mode returns default") {
        bool result = client->getBoolAssignment("", "test-subject", attrs, false);
        CHECK(result == false);

        // Should have logged error
        REQUIRE(mockLogger->errorMessages.size() >= 1);
        bool foundFlagKeyError = false;
        for (const auto& msg : mockLogger->errorMessages) {
            if (msg.find("No flag key provided") != std::string::npos ||
                msg.find("no flag key provided") != std::string::npos) {
                foundFlagKeyError = true;
                break;
            }
        }
        CHECK(foundFlagKeyError);
    }

    SECTION("Empty flag key in non-graceful mode throws") {
        mockLogger->clear();
        client->setIsGracefulFailureMode(false);

        REQUIRE_THROWS_AS(
            client->getBoolAssignment("", "test-subject", attrs, false),
            std::invalid_argument
        );

        // Should have logged error before throwing
        REQUIRE(mockLogger->errorMessages.size() >= 1);
    }
}

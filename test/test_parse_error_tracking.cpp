#include <catch_amalgamated.hpp>
#include <nlohmann/json.hpp>
#include "../src/application_logger.hpp"
#include "../src/bandit_model.hpp"
#include "../src/config_response.hpp"

using namespace eppoclient;
using json = nlohmann::json;

// Test logger that captures warnings
class TestLogger : public ApplicationLogger {
public:
    std::vector<std::string> warnings;
    std::vector<std::string> errors;

    void debug(const std::string&) override {}
    void info(const std::string&) override {}
    void warn(const std::string& message) override { warnings.push_back(message); }
    void error(const std::string& message) override { errors.push_back(message); }
};

TEST_CASE("ConfigResponse tracks parse errors", "[parse_errors]") {
    json invalidConfig = {
        {"flags",
         {{"valid_flag",
           {{"key", "valid_flag"},
            {"enabled", true},
            {"variationType", "STRING"},
            {"variations", {{"control", {{"key", "control"}, {"value", "control_value"}}}}},
            {"allocations", json::array()},
            {"totalShards", 10000}}},
          {"invalid_flag",
           {{"key", "invalid_flag"},
            // Missing required field "enabled"
            {"variationType", "STRING"},
            {"variations", {{"control", {{"key", "control"}, {"value", "control_value"}}}}},
            {"allocations", json::array()}}},
          {"another_invalid",
           {{"key", "another_invalid"},
            {"enabled", "not_a_boolean"},  // Wrong type
            {"variationType", "STRING"},
            {"variations", {{"control", {{"key", "control"}, {"value", "control_value"}}}}},
            {"allocations", json::array()}}}}},
        {"bandits", json::object()}};

    ConfigResponse cr = invalidConfig.get<ConfigResponse>();

    SECTION("Parse stats are populated") {
        CHECK(cr.parseStats.failedFlags == 2);
        CHECK(cr.parseStats.errors.size() == 2);
        CHECK(cr.flags.size() == 1);  // Only valid_flag loaded
        CHECK(cr.flags.count("valid_flag") == 1);
        CHECK(cr.flags.count("invalid_flag") == 0);
        CHECK(cr.flags.count("another_invalid") == 0);
    }

    SECTION("Error messages contain useful information") {
        bool foundEnabledError = false;
        bool foundBooleanError = false;

        for (const auto& error : cr.parseStats.errors) {
            if (error.find("invalid_flag") != std::string::npos &&
                error.find("enabled") != std::string::npos) {
                foundEnabledError = true;
            }
            if (error.find("another_invalid") != std::string::npos &&
                error.find("boolean") != std::string::npos) {
                foundBooleanError = true;
            }
        }

        CHECK(foundEnabledError);
        CHECK(foundBooleanError);
    }

    SECTION("Logger helper function works") {
        TestLogger logger;
        logParseErrors(cr, &logger);

        CHECK_FALSE(logger.warnings.empty());

        // Should have summary + individual errors
        bool foundSummary = false;
        for (const auto& warning : logger.warnings) {
            if (warning.find("2 flag(s) failed") != std::string::npos) {
                foundSummary = true;
            }
        }
        CHECK(foundSummary);
    }

    SECTION("Logger with null pointer doesn't crash") {
        REQUIRE_NOTHROW(logParseErrors(cr, nullptr));
    }

    SECTION("Logger with no errors doesn't log anything") {
        json validConfig = {
            {"flags",
             {{"valid_flag",
               {{"key", "valid_flag"},
                {"enabled", true},
                {"variationType", "STRING"},
                {"variations", {{"control", {{"key", "control"}, {"value", "control_value"}}}}},
                {"allocations", json::array()},
                {"totalShards", 10000}}}}},
            {"bandits", json::object()}};

        ConfigResponse validCr = validConfig.get<ConfigResponse>();
        TestLogger logger;
        logParseErrors(validCr, &logger);

        CHECK(logger.warnings.empty());
        CHECK(validCr.parseStats.failedFlags == 0);
        CHECK(validCr.parseStats.errors.empty());
    }
}

TEST_CASE("BanditResponse tracks parse errors", "[parse_errors]") {
    json invalidBandits = {{"bandits",
                            {{"valid_bandit",
                              {{"banditKey", "valid_bandit"},
                               {"modelName", "falcon"},
                               {"modelVersion", "v1"},
                               {"modelData",
                                {{"gamma", 1.0},
                                 {"defaultActionScore", 0.0},
                                 {"actionProbabilityFloor", 0.0},
                                 {"coefficients", json::object()}}}}},
                             {"invalid_bandit",
                              {{"banditKey", "invalid_bandit"},
                               // Missing modelName
                               {"modelVersion", "v1"},
                               {"modelData",
                                {{"gamma", 1.0},
                                 {"defaultActionScore", 0.0},
                                 {"actionProbabilityFloor", 0.0},
                                 {"coefficients", json::object()}}}}}}},
                           {"updatedAt", "2023-01-01T00:00:00.000Z"}};

    BanditResponse br = invalidBandits.get<BanditResponse>();

    SECTION("Parse stats are populated") {
        CHECK(br.parseStats.failedBandits == 1);
        CHECK(br.parseStats.errors.size() == 1);
        CHECK(br.bandits.size() == 1);  // Only valid_bandit loaded
        CHECK(br.bandits.count("valid_bandit") == 1);
        CHECK(br.bandits.count("invalid_bandit") == 0);
    }

    SECTION("Error messages contain useful information") {
        bool foundModelNameError = false;

        for (const auto& error : br.parseStats.errors) {
            if (error.find("invalid_bandit") != std::string::npos &&
                error.find("modelName") != std::string::npos) {
                foundModelNameError = true;
            }
        }

        CHECK(foundModelNameError);
    }

    SECTION("Logger helper function works") {
        TestLogger logger;
        logParseErrors(br, &logger);

        CHECK_FALSE(logger.warnings.empty());

        // Should have summary message
        bool foundSummary = false;
        for (const auto& warning : logger.warnings) {
            if (warning.find("1 bandit(s) failed") != std::string::npos) {
                foundSummary = true;
            }
        }
        CHECK(foundSummary);
    }
}

TEST_CASE("Parse error logging limits output", "[parse_errors]") {
    json configWithManyErrors = {{"flags", json::object()}, {"bandits", json::object()}};

    // Add 15 invalid flags
    for (int i = 0; i < 15; i++) {
        std::string flagKey = "invalid_flag_" + std::to_string(i);
        configWithManyErrors["flags"][flagKey] = {{"key", flagKey},
                                                  // Missing enabled field
                                                  {"variationType", "STRING"},
                                                  {"variations", json::object()},
                                                  {"allocations", json::array()}};
    }

    ConfigResponse cr = configWithManyErrors.get<ConfigResponse>();

    SECTION("All errors are tracked in parseStats") {
        CHECK(cr.parseStats.failedFlags == 15);
        CHECK(cr.parseStats.errors.size() == 15);
    }

    SECTION("Logger limits output to 10 errors") {
        TestLogger logger;
        logParseErrors(cr, &logger);

        // Should have: 1 summary + 10 errors + 1 "and X more" message
        CHECK(logger.warnings.size() == 12);

        // Check for the "and X more" message
        bool foundMoreMessage = false;
        for (const auto& warning : logger.warnings) {
            if (warning.find("and 5 more error(s)") != std::string::npos) {
                foundMoreMessage = true;
            }
        }
        CHECK(foundMoreMessage);
    }
}

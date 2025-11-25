#include <catch_amalgamated.hpp>
#include "../src/client.hpp"
#include "../src/config_response.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <memory>

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

    // Mock bandit logger
    class MockBanditLogger : public BanditLogger {
    public:
        std::vector<BanditEvent> loggedEvents;

        void logBanditAction(const BanditEvent& event) override {
            loggedEvents.push_back(event);
        }

        void clear() {
            loggedEvents.clear();
        }
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
}

TEST_CASE("Bandit Action Details - getBanditActionDetails with actions", "[bandit-action-details]") {
    ConfigurationStore configStore;
    ConfigResponse ufc = loadFlagsConfiguration("test/data/ufc/bandit-flags-v1.json");
    configStore.setConfiguration(Configuration(ufc));

    auto mockAppLogger = std::make_shared<MockApplicationLogger>();
    auto mockBanditLogger = std::make_shared<MockBanditLogger>();

    EppoClient client(configStore, nullptr, mockBanditLogger, mockAppLogger);

    SECTION("Successful bandit action with details") {
        ContextAttributes subjectAttrs;
        subjectAttrs.numericAttributes["age"] = 30.0;

        std::map<std::string, ContextAttributes> actions;
        ContextAttributes action1, action2;
        action1.categoricalAttributes["color"] = "red";
        action2.categoricalAttributes["color"] = "blue";
        actions["action1"] = action1;
        actions["action2"] = action2;

        mockBanditLogger->clear();

        auto result = client.getBanditActionDetails(
            "banner_bandit_flag",
            "test-subject",
            subjectAttrs,
            actions,
            "default-variation"
        );

        // Check that result has evaluation details
        REQUIRE(result.evaluationDetails.has_value());
        CHECK(result.evaluationDetails->flagKey == "banner_bandit_flag");
        CHECK(result.evaluationDetails->subjectKey == "test-subject");

        // Check flag evaluation code
        REQUIRE(result.evaluationDetails->flagEvaluationCode.has_value());

        // Check bandit evaluation code is set
        REQUIRE(result.evaluationDetails->banditEvaluationCode.has_value());

        // If we got a bandit action, check that action is set
        if (result.action.has_value()) {
            CHECK(*result.evaluationDetails->banditEvaluationCode == BanditEvaluationCode::MATCH);
            REQUIRE(result.evaluationDetails->banditAction.has_value());
            CHECK(*result.evaluationDetails->banditAction == *result.action);

            // Verify bandit event was logged
            CHECK(mockBanditLogger->loggedEvents.size() == 1);
        }
    }

    SECTION("Bandit action details with timestamp") {
        ContextAttributes subjectAttrs;
        std::map<std::string, ContextAttributes> actions;
        actions["action1"] = ContextAttributes();
        actions["action2"] = ContextAttributes();

        auto result = client.getBanditActionDetails(
            "banner_bandit_flag",
            "test-subject",
            subjectAttrs,
            actions,
            "default"
        );

        REQUIRE(result.evaluationDetails.has_value());
        CHECK_FALSE(result.evaluationDetails->timestamp.empty());

        // Verify timestamp format
        CHECK(result.evaluationDetails->timestamp.find("T") != std::string::npos);
        CHECK(result.evaluationDetails->timestamp.find("Z") != std::string::npos);
    }
}

TEST_CASE("Bandit Action Details - No actions supplied", "[bandit-action-details]") {
    ConfigurationStore configStore;
    ConfigResponse ufc = loadFlagsConfiguration("test/data/ufc/bandit-flags-v1.json");
    configStore.setConfiguration(Configuration(ufc));

    auto mockBanditLogger = std::make_shared<MockBanditLogger>();
    EppoClient client(configStore, nullptr, mockBanditLogger, nullptr);

    SECTION("No actions returns variation with NO_ACTIONS_SUPPLIED code") {
        ContextAttributes subjectAttrs;
        std::map<std::string, ContextAttributes> emptyActions;

        mockBanditLogger->clear();

        auto result = client.getBanditActionDetails(
            "banner_bandit_flag",
            "test-subject",
            subjectAttrs,
            emptyActions,  // Empty actions map
            "default-variation"
        );

        // Check evaluation details
        REQUIRE(result.evaluationDetails.has_value());

        // Check bandit evaluation code indicates no actions
        REQUIRE(result.evaluationDetails->banditEvaluationCode.has_value());
        CHECK(*result.evaluationDetails->banditEvaluationCode == BanditEvaluationCode::NO_ACTIONS_SUPPLIED_FOR_BANDIT);

        // Check no action was returned
        CHECK_FALSE(result.action.has_value());

        // Verify no bandit event was logged
        CHECK(mockBanditLogger->loggedEvents.empty());
    }
}

TEST_CASE("Bandit Action Details - Non-bandit variation", "[bandit-action-details]") {
    ConfigurationStore configStore;
    ConfigResponse ufc = loadFlagsConfiguration("test/data/ufc/bandit-flags-v1.json");
    configStore.setConfiguration(Configuration(ufc));

    EppoClient client(configStore, nullptr, nullptr, nullptr);

    SECTION("Non-bandit flag returns NON_BANDIT_VARIATION code") {
        ContextAttributes subjectAttrs;
        std::map<std::string, ContextAttributes> actions;
        actions["action1"] = ContextAttributes();

        // Use a non-bandit flag
        auto result = client.getBanditActionDetails(
            "non_bandit_flag",
            "test-subject",
            subjectAttrs,
            actions,
            "default"
        );

        REQUIRE(result.evaluationDetails.has_value());

        // Check bandit evaluation code indicates non-bandit variation
        REQUIRE(result.evaluationDetails->banditEvaluationCode.has_value());
        CHECK(*result.evaluationDetails->banditEvaluationCode == BanditEvaluationCode::NON_BANDIT_VARIATION);

        // Check no action was returned
        CHECK_FALSE(result.action.has_value());
    }
}

TEST_CASE("Bandit Action Details - Non-existent flag", "[bandit-action-details]") {
    ConfigurationStore configStore;
    ConfigResponse ufc = loadFlagsConfiguration("test/data/ufc/bandit-flags-v1.json");
    configStore.setConfiguration(Configuration(ufc));

    EppoClient client(configStore, nullptr, nullptr, nullptr);

    SECTION("Non-existent flag returns default with FLAG_UNRECOGNIZED_OR_DISABLED") {
        ContextAttributes subjectAttrs;
        std::map<std::string, ContextAttributes> actions;
        actions["action1"] = ContextAttributes();

        auto result = client.getBanditActionDetails(
            "non-existent-bandit-flag",
            "test-subject",
            subjectAttrs,
            actions,
            "fallback"
        );

        CHECK(result.variation == "fallback");

        REQUIRE(result.evaluationDetails.has_value());
        REQUIRE(result.evaluationDetails->flagEvaluationCode.has_value());
        CHECK(*result.evaluationDetails->flagEvaluationCode == FlagEvaluationCode::FLAG_UNRECOGNIZED_OR_DISABLED);
    }
}

TEST_CASE("Bandit Action Details - Bandit key in details", "[bandit-action-details]") {
    ConfigurationStore configStore;
    ConfigResponse ufc = loadFlagsConfiguration("test/data/ufc/bandit-flags-v1.json");
    configStore.setConfiguration(Configuration(ufc));

    EppoClient client(configStore, nullptr, nullptr, nullptr);

    SECTION("Bandit key is included in evaluation details") {
        ContextAttributes subjectAttrs;
        std::map<std::string, ContextAttributes> actions;
        actions["action1"] = ContextAttributes();
        actions["action2"] = ContextAttributes();

        auto result = client.getBanditActionDetails(
            "banner_bandit_flag",
            "test-subject",
            subjectAttrs,
            actions,
            "default"
        );

        REQUIRE(result.evaluationDetails.has_value());

        // If we got a bandit match, bandit key should be present
        if (result.evaluationDetails->banditEvaluationCode.has_value() &&
            *result.evaluationDetails->banditEvaluationCode == BanditEvaluationCode::MATCH) {
            REQUIRE(result.evaluationDetails->banditKey.has_value());
            CHECK_FALSE(result.evaluationDetails->banditKey->empty());
        }
    }
}

TEST_CASE("Bandit Action Details - Multiple calls preserve details", "[bandit-action-details]") {
    ConfigurationStore configStore;
    ConfigResponse ufc = loadFlagsConfiguration("test/data/ufc/bandit-flags-v1.json");
    configStore.setConfiguration(Configuration(ufc));

    EppoClient client(configStore, nullptr, nullptr, nullptr);

    SECTION("Multiple bandit calls produce independent details") {
        ContextAttributes subjectAttrs1, subjectAttrs2;
        subjectAttrs1.numericAttributes["age"] = 25.0;
        subjectAttrs2.numericAttributes["age"] = 45.0;

        std::map<std::string, ContextAttributes> actions;
        actions["action1"] = ContextAttributes();
        actions["action2"] = ContextAttributes();

        auto result1 = client.getBanditActionDetails(
            "banner_bandit_flag",
            "user1",
            subjectAttrs1,
            actions,
            "default"
        );

        auto result2 = client.getBanditActionDetails(
            "banner_bandit_flag",
            "user2",
            subjectAttrs2,
            actions,
            "default"
        );

        // Check both have evaluation details
        REQUIRE(result1.evaluationDetails.has_value());
        REQUIRE(result2.evaluationDetails.has_value());

        // Verify they have different subject keys
        CHECK(result1.evaluationDetails->subjectKey == "user1");
        CHECK(result2.evaluationDetails->subjectKey == "user2");

        // Verify subject attributes are different
        CHECK(std::holds_alternative<double>(result1.evaluationDetails->subjectAttributes["age"]));
        CHECK(std::get<double>(result1.evaluationDetails->subjectAttributes["age"]) == 25.0);
        CHECK(std::holds_alternative<double>(result2.evaluationDetails->subjectAttributes["age"]));
        CHECK(std::get<double>(result2.evaluationDetails->subjectAttributes["age"]) == 45.0);
    }
}

TEST_CASE("Bandit Action Details - Empty subject key handling", "[bandit-action-details]") {
    ConfigurationStore configStore;
    ConfigResponse ufc = loadFlagsConfiguration("test/data/ufc/bandit-flags-v1.json");
    configStore.setConfiguration(Configuration(ufc));

    EppoClient client(configStore, nullptr, nullptr, nullptr);

    SECTION("Empty subject key returns default with error details") {
        std::map<std::string, ContextAttributes> actions;
        actions["action1"] = ContextAttributes();

        auto result = client.getBanditActionDetails(
            "banner_bandit_flag",
            "",  // Empty subject key
            ContextAttributes(),
            actions,
            "default"
        );

        CHECK(result.variation == "default");

        REQUIRE(result.evaluationDetails.has_value());
        REQUIRE(result.evaluationDetails->flagEvaluationCode.has_value());
        CHECK(*result.evaluationDetails->flagEvaluationCode == FlagEvaluationCode::ASSIGNMENT_ERROR);
    }
}

TEST_CASE("Bandit Action Details - Variation and action consistency", "[bandit-action-details]") {
    ConfigurationStore configStore;
    ConfigResponse ufc = loadFlagsConfiguration("test/data/ufc/bandit-flags-v1.json");
    configStore.setConfiguration(Configuration(ufc));

    EppoClient client(configStore, nullptr, nullptr, nullptr);

    SECTION("Returned action matches action in evaluation details") {
        ContextAttributes subjectAttrs;
        std::map<std::string, ContextAttributes> actions;
        actions["action1"] = ContextAttributes();
        actions["action2"] = ContextAttributes();

        auto result = client.getBanditActionDetails(
            "banner_bandit_flag",
            "test-subject",
            subjectAttrs,
            actions,
            "default"
        );

        REQUIRE(result.evaluationDetails.has_value());

        // If action was returned, it should match the one in details
        if (result.action.has_value()) {
            REQUIRE(result.evaluationDetails->banditAction.has_value());
            CHECK(*result.action == *result.evaluationDetails->banditAction);
        } else {
            // If no action, details should also not have an action
            CHECK_FALSE(result.evaluationDetails->banditAction.has_value());
        }

        // Variation should match variation key in details (if present)
        if (result.evaluationDetails->variationKey.has_value()) {
            // The variation string should match the variation key
            CHECK_FALSE(result.variation.empty());
        }
    }
}

TEST_CASE("Bandit Action Details - Bandit logging still works", "[bandit-action-details]") {
    ConfigurationStore configStore;
    ConfigResponse ufc = loadFlagsConfiguration("test/data/ufc/bandit-flags-v1.json");
    configStore.setConfiguration(Configuration(ufc));

    auto mockBanditLogger = std::make_shared<MockBanditLogger>();
    EppoClient client(configStore, nullptr, mockBanditLogger, nullptr);

    SECTION("Bandit events are logged with details methods") {
        ContextAttributes subjectAttrs;
        std::map<std::string, ContextAttributes> actions;
        actions["action1"] = ContextAttributes();
        actions["action2"] = ContextAttributes();

        mockBanditLogger->clear();

        auto result = client.getBanditActionDetails(
            "banner_bandit_flag",
            "test-subject",
            subjectAttrs,
            actions,
            "default"
        );

        // If a bandit action was selected, event should be logged
        if (result.action.has_value()) {
            CHECK(mockBanditLogger->loggedEvents.size() == 1);

            if (!mockBanditLogger->loggedEvents.empty()) {
                const auto& event = mockBanditLogger->loggedEvents[0];
                CHECK(event.flagKey == "banner_bandit_flag");
                CHECK(event.subject == "test-subject");
                CHECK(event.action == *result.action);
            }
        } else {
            // If no action (e.g., non-bandit variation), no event should be logged
            CHECK(mockBanditLogger->loggedEvents.empty());
        }
    }
}

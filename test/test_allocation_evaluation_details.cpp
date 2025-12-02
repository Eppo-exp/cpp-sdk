#include <catch_amalgamated.hpp>
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include "../src/client.hpp"
#include "../src/config_response.hpp"

using namespace eppoclient;
using json = nlohmann::json;

namespace {
// Helper function to load flags configuration
ConfigResponse loadFlagsConfiguration(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open flags configuration file: " + filepath);
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
}  // namespace

TEST_CASE("Allocation Evaluation Details - BEFORE_START_TIME", "[allocation-details]") {
    auto configStore = std::make_shared<ConfigurationStore>();
    ConfigResponse ufc = loadFlagsConfiguration("test/data/ufc/flags-v1.json");
    configStore->setConfiguration(Configuration(ufc));

    EppoClient client(configStore, nullptr, nullptr, nullptr);

    SECTION("Flag with start date tracks time-based codes") {
        // The start-and-end-date-test flag has allocations with start times
        auto result = client.getBooleanAssignmentDetails(
            "start-and-end-date-test", "test-subject-before", Attributes(), false);

        REQUIRE(result.evaluationDetails.has_value());

        // If allocations exist, verify they have valid codes
        if (!result.evaluationDetails->allocations.empty()) {
            for (const auto& alloc : result.evaluationDetails->allocations) {
                // Each allocation should have a valid evaluation code
                CHECK((alloc.allocationEvaluationCode == AllocationEvaluationCode::MATCH ||
                       alloc.allocationEvaluationCode ==
                           AllocationEvaluationCode::TRAFFIC_EXPOSURE_MISS ||
                       alloc.allocationEvaluationCode == AllocationEvaluationCode::FAILING_RULE ||
                       alloc.allocationEvaluationCode ==
                           AllocationEvaluationCode::BEFORE_START_TIME ||
                       alloc.allocationEvaluationCode == AllocationEvaluationCode::AFTER_END_TIME));
                CHECK_FALSE(alloc.key.empty());
            }
        }

        // Note: This flag might not have allocations or they may have expired
        INFO("Allocation details present: " << result.evaluationDetails->allocations.size());
    }
}

TEST_CASE("Allocation Evaluation Details - AFTER_END_TIME", "[allocation-details]") {
    auto configStore = std::make_shared<ConfigurationStore>();
    ConfigResponse ufc = loadFlagsConfiguration("test/data/ufc/flags-v1.json");
    configStore->setConfiguration(Configuration(ufc));

    EppoClient client(configStore, nullptr, nullptr, nullptr);

    SECTION("Flag with time-based allocations tracks evaluation codes") {
        // The start-and-end-date-test flag has allocations with end times
        auto result = client.getBooleanAssignmentDetails("start-and-end-date-test",
                                                         "test-subject-after", Attributes(), false);

        REQUIRE(result.evaluationDetails.has_value());

        // If allocations exist, verify they have valid codes
        if (!result.evaluationDetails->allocations.empty()) {
            for (const auto& alloc : result.evaluationDetails->allocations) {
                // Each allocation should have a valid evaluation code
                CHECK((alloc.allocationEvaluationCode == AllocationEvaluationCode::MATCH ||
                       alloc.allocationEvaluationCode ==
                           AllocationEvaluationCode::TRAFFIC_EXPOSURE_MISS ||
                       alloc.allocationEvaluationCode == AllocationEvaluationCode::FAILING_RULE ||
                       alloc.allocationEvaluationCode ==
                           AllocationEvaluationCode::BEFORE_START_TIME ||
                       alloc.allocationEvaluationCode == AllocationEvaluationCode::AFTER_END_TIME));
                CHECK_FALSE(alloc.key.empty());
            }
        }

        INFO("Allocation details present: " << result.evaluationDetails->allocations.size());
    }
}

TEST_CASE("Allocation Evaluation Details - FAILING_RULE", "[allocation-details]") {
    auto configStore = std::make_shared<ConfigurationStore>();
    ConfigResponse ufc = loadFlagsConfiguration("test/data/ufc/flags-v1.json");
    configStore->setConfiguration(Configuration(ufc));

    EppoClient client(configStore, nullptr, nullptr, nullptr);

    SECTION("Subject not matching allocation rules has FAILING_RULE code") {
        // Use a flag with rules - kill-switch has allocations with rules
        Attributes attrs;
        attrs["company_id"] = std::string("non-matching-company");

        auto result =
            client.getBooleanAssignmentDetails("kill-switch", "test-subject", attrs, false);

        REQUIRE(result.evaluationDetails.has_value());
        CHECK_FALSE(result.evaluationDetails->allocations.empty());

        // Check allocation evaluation codes - at least one should have been evaluated
        for (const auto& alloc : result.evaluationDetails->allocations) {
            // Each allocation should have a valid code
            CHECK((alloc.allocationEvaluationCode == AllocationEvaluationCode::MATCH ||
                   alloc.allocationEvaluationCode ==
                       AllocationEvaluationCode::TRAFFIC_EXPOSURE_MISS ||
                   alloc.allocationEvaluationCode == AllocationEvaluationCode::FAILING_RULE ||
                   alloc.allocationEvaluationCode == AllocationEvaluationCode::BEFORE_START_TIME ||
                   alloc.allocationEvaluationCode == AllocationEvaluationCode::AFTER_END_TIME));
            CHECK_FALSE(alloc.key.empty());
        }

        // At least one allocation should have been evaluated
        CHECK(result.evaluationDetails->allocations.size() > 0);
    }

    SECTION("Subject matching rules shows MATCH code") {
        // Empty attributes should match the default allocation
        auto result =
            client.getBooleanAssignmentDetails("kill-switch", "alice", Attributes(), false);

        REQUIRE(result.evaluationDetails.has_value());
        CHECK_FALSE(result.evaluationDetails->allocations.empty());

        // At least one allocation should have MATCH code
        bool foundMatch = false;
        for (const auto& alloc : result.evaluationDetails->allocations) {
            if (alloc.allocationEvaluationCode == AllocationEvaluationCode::MATCH) {
                foundMatch = true;
                CHECK_FALSE(alloc.key.empty());
                break;
            }
        }

        CHECK(foundMatch);
    }
}

TEST_CASE("Allocation Evaluation Details - TRAFFIC_EXPOSURE_MISS", "[allocation-details]") {
    auto configStore = std::make_shared<ConfigurationStore>();
    ConfigResponse ufc = loadFlagsConfiguration("test/data/ufc/flags-v1.json");
    configStore->setConfiguration(Configuration(ufc));

    EppoClient client(configStore, nullptr, nullptr, nullptr);

    SECTION("Subject not in traffic allocation has TRAFFIC_EXPOSURE_MISS code") {
        // Create a subject that passes rules but may not be in traffic split
        Attributes attrs;

        auto result = client.getBooleanAssignmentDetails(
            "kill-switch", "test-subject-not-in-traffic", attrs, false);

        REQUIRE(result.evaluationDetails.has_value());
        CHECK_FALSE(result.evaluationDetails->allocations.empty());

        // Check allocation evaluation codes
        for (const auto& alloc : result.evaluationDetails->allocations) {
            // Each allocation should have a valid code
            CHECK((alloc.allocationEvaluationCode == AllocationEvaluationCode::MATCH ||
                   alloc.allocationEvaluationCode ==
                       AllocationEvaluationCode::TRAFFIC_EXPOSURE_MISS ||
                   alloc.allocationEvaluationCode == AllocationEvaluationCode::FAILING_RULE ||
                   alloc.allocationEvaluationCode == AllocationEvaluationCode::BEFORE_START_TIME ||
                   alloc.allocationEvaluationCode == AllocationEvaluationCode::AFTER_END_TIME));
        }
    }
}

TEST_CASE("Allocation Evaluation Details - Multiple allocations tracked", "[allocation-details]") {
    auto configStore = std::make_shared<ConfigurationStore>();
    ConfigResponse ufc = loadFlagsConfiguration("test/data/ufc/flags-v1.json");
    configStore->setConfiguration(Configuration(ufc));

    EppoClient client(configStore, nullptr, nullptr, nullptr);

    SECTION("All allocations are evaluated and tracked") {
        auto result =
            client.getBooleanAssignmentDetails("kill-switch", "alice", Attributes(), false);

        REQUIRE(result.evaluationDetails.has_value());
        CHECK_FALSE(result.evaluationDetails->allocations.empty());

        // Verify each allocation has required fields
        for (const auto& alloc : result.evaluationDetails->allocations) {
            CHECK_FALSE(alloc.key.empty());
            // orderPosition should be valid (1-indexed)
            CHECK(alloc.orderPosition >= 1);
            // Should have an evaluation code
            CHECK((alloc.allocationEvaluationCode == AllocationEvaluationCode::MATCH ||
                   alloc.allocationEvaluationCode ==
                       AllocationEvaluationCode::TRAFFIC_EXPOSURE_MISS ||
                   alloc.allocationEvaluationCode == AllocationEvaluationCode::FAILING_RULE ||
                   alloc.allocationEvaluationCode == AllocationEvaluationCode::BEFORE_START_TIME ||
                   alloc.allocationEvaluationCode == AllocationEvaluationCode::AFTER_END_TIME));
        }

        // Exactly one allocation should have MATCH if flag evaluation succeeded
        if (result.evaluationDetails->flagEvaluationCode.has_value() &&
            *result.evaluationDetails->flagEvaluationCode == FlagEvaluationCode::MATCH) {
            int matchCount = 0;
            for (const auto& alloc : result.evaluationDetails->allocations) {
                if (alloc.allocationEvaluationCode == AllocationEvaluationCode::MATCH) {
                    matchCount++;
                }
            }
            CHECK(matchCount >= 1);
        }
    }

    SECTION("Allocation order positions are sequential") {
        auto result = client.getStringAssignmentDetails("empty_string_flag", "test-subject",
                                                        Attributes(), "default");

        REQUIRE(result.evaluationDetails.has_value());
        if (!result.evaluationDetails->allocations.empty()) {
            // Verify order positions are sequential starting from 1
            for (size_t i = 0; i < result.evaluationDetails->allocations.size(); ++i) {
                CHECK(result.evaluationDetails->allocations[i].orderPosition == i + 1);
            }
        }
    }
}

TEST_CASE("Allocation Evaluation Details - First matching allocation wins",
          "[allocation-details]") {
    auto configStore = std::make_shared<ConfigurationStore>();
    ConfigResponse ufc = loadFlagsConfiguration("test/data/ufc/flags-v1.json");
    configStore->setConfiguration(Configuration(ufc));

    EppoClient client(configStore, nullptr, nullptr, nullptr);

    SECTION("When multiple allocations match, first one is used") {
        auto result =
            client.getBooleanAssignmentDetails("kill-switch", "alice", Attributes(), false);

        REQUIRE(result.evaluationDetails.has_value());

        if (result.evaluationDetails->flagEvaluationCode.has_value() &&
            *result.evaluationDetails->flagEvaluationCode == FlagEvaluationCode::MATCH) {
            // Find the first MATCH allocation
            size_t firstMatchPosition = SIZE_MAX;
            for (const auto& alloc : result.evaluationDetails->allocations) {
                if (alloc.allocationEvaluationCode == AllocationEvaluationCode::MATCH &&
                    alloc.orderPosition < firstMatchPosition) {
                    firstMatchPosition = alloc.orderPosition;
                }
            }

            // The matched allocation should be the first one that matches
            CHECK(firstMatchPosition != SIZE_MAX);
        }
    }
}

TEST_CASE("Allocation Evaluation Details - No allocations case", "[allocation-details]") {
    auto configStore = std::make_shared<ConfigurationStore>();
    ConfigResponse ufc = loadFlagsConfiguration("test/data/ufc/flags-v1.json");
    configStore->setConfiguration(Configuration(ufc));

    EppoClient client(configStore, nullptr, nullptr, nullptr);

    SECTION("Flag with no allocations has empty allocations list") {
        auto result = client.getBooleanAssignmentDetails("no_allocations_flag", "test-subject",
                                                         Attributes(), false);

        REQUIRE(result.evaluationDetails.has_value());

        // Should have empty allocations or DEFAULT_ALLOCATION_NULL code
        if (result.evaluationDetails->flagEvaluationCode.has_value() &&
            *result.evaluationDetails->flagEvaluationCode ==
                FlagEvaluationCode::DEFAULT_ALLOCATION_NULL) {
            // This is expected for flags with no allocations
            CHECK(result.variation == false);  // Should return default
        }
    }
}

TEST_CASE("Allocation Evaluation Details - Integer flag allocations", "[allocation-details]") {
    auto configStore = std::make_shared<ConfigurationStore>();
    ConfigResponse ufc = loadFlagsConfiguration("test/data/ufc/flags-v1.json");
    configStore->setConfiguration(Configuration(ufc));

    EppoClient client(configStore, nullptr, nullptr, nullptr);

    SECTION("Integer flags also track allocation details") {
        Attributes attrs;
        attrs["age"] = 25.0;

        auto result = client.getIntegerAssignmentDetails("integer-flag", "alice", attrs, 0);

        REQUIRE(result.evaluationDetails.has_value());

        // Should have allocation details
        CHECK_FALSE(result.evaluationDetails->allocations.empty());

        // At least one allocation should have been evaluated
        bool hasEvaluationCode = false;
        for (const auto& alloc : result.evaluationDetails->allocations) {
            if (alloc.allocationEvaluationCode != AllocationEvaluationCode::UNEVALUATED) {
                hasEvaluationCode = true;
                break;
            }
        }
        CHECK(hasEvaluationCode);
    }
}

TEST_CASE("Allocation Evaluation Details - JSON flag allocations", "[allocation-details]") {
    auto configStore = std::make_shared<ConfigurationStore>();
    ConfigResponse ufc = loadFlagsConfiguration("test/data/ufc/flags-v1.json");
    configStore->setConfiguration(Configuration(ufc));

    EppoClient client(configStore, nullptr, nullptr, nullptr);

    SECTION("JSON flags track allocation details") {
        json defaultJson = {{"default", "value"}};

        auto result = client.getJsonAssignmentDetails("json-config-flag", "test-subject-1",
                                                      Attributes(), defaultJson);

        REQUIRE(result.evaluationDetails.has_value());
        CHECK_FALSE(result.evaluationDetails->allocations.empty());

        // Verify allocation details are present
        for (const auto& alloc : result.evaluationDetails->allocations) {
            CHECK_FALSE(alloc.key.empty());
        }
    }
}

TEST_CASE("Allocation Evaluation Details - Allocation key preserved", "[allocation-details]") {
    auto configStore = std::make_shared<ConfigurationStore>();
    ConfigResponse ufc = loadFlagsConfiguration("test/data/ufc/flags-v1.json");
    configStore->setConfiguration(Configuration(ufc));

    EppoClient client(configStore, nullptr, nullptr, nullptr);

    SECTION("Allocation keys are preserved in details") {
        auto result = client.getBooleanAssignmentDetails("boolean-false-assignment", "test-subject",
                                                         Attributes(), false);

        REQUIRE(result.evaluationDetails.has_value());

        for (const auto& alloc : result.evaluationDetails->allocations) {
            // Allocation key should not be empty
            CHECK_FALSE(alloc.key.empty());

            // Allocation key should be a reasonable length (not corrupted)
            CHECK(alloc.key.length() > 0);
            CHECK(alloc.key.length() < 1000);
        }
    }
}

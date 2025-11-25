#include <catch_amalgamated.hpp>
#include "../src/client.hpp"
#include "../src/config_response.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <iostream>

using namespace eppoclient;
using json = nlohmann::json;
namespace fs = std::filesystem;

// Structure to hold expected allocation evaluation data
struct ExpectedAllocationEvaluation {
    std::string key;
    std::string allocationEvaluationCode;
    size_t orderPosition;
};

// Structure to hold evaluation details test data
struct EvaluationDetailsTestSubject {
    std::string subjectKey;
    Attributes subjectAttributes;
    json expectedAssignment;
    std::string expectedFlagEvaluationCode;
    std::optional<ExpectedAllocationEvaluation> expectedMatchedAllocation;
    std::vector<ExpectedAllocationEvaluation> expectedUnmatchedAllocations;
    std::vector<ExpectedAllocationEvaluation> expectedUnevaluatedAllocations;
};

// Structure to hold test case data with evaluation details
struct EvaluationDetailsTestCase {
    std::string flag;
    VariationType variationType;
    json defaultValue;
    std::vector<EvaluationDetailsTestSubject> subjects;
    std::string filename; // For better error messages
};

// Helper function to load flags configuration from JSON file
static ConfigResponse loadFlagsConfiguration(const std::string& filepath) {
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
static Attributes parseAttributes(const json& attrJson) {
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
            // Skip null values or handle them specially
            continue;
        }
    }

    return attributes;
}


// Helper function to load a single test case from JSON file
static EvaluationDetailsTestCase loadTestCase(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open test case file: " + filepath);
    }

    json j;
    file >> j;

    EvaluationDetailsTestCase testCase;
    testCase.flag = j["flag"].get<std::string>();
    testCase.variationType = j["variationType"].get<VariationType>();
    testCase.defaultValue = j["defaultValue"];
    testCase.filename = fs::path(filepath).filename().string();

    // Parse subjects
    for (const auto& subjectJson : j["subjects"]) {
        EvaluationDetailsTestSubject subject;
        subject.subjectKey = subjectJson["subjectKey"].get<std::string>();
        subject.subjectAttributes = parseAttributes(subjectJson["subjectAttributes"]);
        subject.expectedAssignment = subjectJson["assignment"];

        // Extract flagEvaluationCode from evaluationDetails
        if (subjectJson.contains("evaluationDetails") &&
            subjectJson["evaluationDetails"].contains("flagEvaluationCode")) {
            subject.expectedFlagEvaluationCode =
                subjectJson["evaluationDetails"]["flagEvaluationCode"].get<std::string>();
        } else {
            // If no evaluation details, skip this subject
            continue;
        }

        // Extract allocation evaluation codes from evaluationDetails
        const auto& evalDetails = subjectJson["evaluationDetails"];

        // Parse matchedAllocation
        if (evalDetails.contains("matchedAllocation") && !evalDetails["matchedAllocation"].is_null()) {
            ExpectedAllocationEvaluation matched;
            matched.key = evalDetails["matchedAllocation"]["key"].get<std::string>();
            matched.allocationEvaluationCode = evalDetails["matchedAllocation"]["allocationEvaluationCode"].get<std::string>();
            matched.orderPosition = evalDetails["matchedAllocation"]["orderPosition"].get<size_t>();
            subject.expectedMatchedAllocation = matched;
        }

        // Parse unmatchedAllocations
        if (evalDetails.contains("unmatchedAllocations")) {
            for (const auto& alloc : evalDetails["unmatchedAllocations"]) {
                ExpectedAllocationEvaluation unmatched;
                unmatched.key = alloc["key"].get<std::string>();
                unmatched.allocationEvaluationCode = alloc["allocationEvaluationCode"].get<std::string>();
                unmatched.orderPosition = alloc["orderPosition"].get<size_t>();
                subject.expectedUnmatchedAllocations.push_back(unmatched);
            }
        }

        // Parse unevaluatedAllocations
        if (evalDetails.contains("unevaluatedAllocations")) {
            for (const auto& alloc : evalDetails["unevaluatedAllocations"]) {
                ExpectedAllocationEvaluation unevaluated;
                unevaluated.key = alloc["key"].get<std::string>();
                unevaluated.allocationEvaluationCode = alloc["allocationEvaluationCode"].get<std::string>();
                unevaluated.orderPosition = alloc["orderPosition"].get<size_t>();
                subject.expectedUnevaluatedAllocations.push_back(unevaluated);
            }
        }

        testCase.subjects.push_back(subject);
    }

    return testCase;
}

// Helper function to load all test cases from directory
static std::vector<EvaluationDetailsTestCase> loadAllTestCases(const std::string& directory) {
    std::vector<EvaluationDetailsTestCase> testCases;

    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            std::string filename = entry.path().filename().string();
            // Skip files that don't start with "test-"
            if (filename.find("test-") == 0) {
                try {
                    auto testCase = loadTestCase(entry.path().string());
                    // Only add test cases that have subjects with evaluation details
                    if (!testCase.subjects.empty()) {
                        testCases.push_back(testCase);
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Warning: Failed to load test case " << filename
                              << ": " << e.what() << std::endl;
                }
            }
        }
    }

    return testCases;
}

// Helper function to validate allocation evaluation codes
static void validateAllocationEvaluationCodes(
    const EvaluationDetails& evaluationDetails,
    const EvaluationDetailsTestSubject& subject,
    const std::string& flag
) {
    // Find matched, unmatched, and unevaluated allocations from actual results
    std::optional<AllocationEvaluationDetails> actualMatched;
    std::vector<AllocationEvaluationDetails> actualUnmatched;
    std::vector<AllocationEvaluationDetails> actualUnevaluated;

    for (const auto& alloc : evaluationDetails.allocations) {
        if (alloc.allocationEvaluationCode == AllocationEvaluationCode::MATCH) {
            actualMatched = alloc;
        } else if (alloc.allocationEvaluationCode == AllocationEvaluationCode::UNEVALUATED) {
            actualUnevaluated.push_back(alloc);
        } else {
            actualUnmatched.push_back(alloc);
        }
    }

    // Validate matched allocation
    if (subject.expectedMatchedAllocation.has_value()) {
        REQUIRE(actualMatched.has_value());
        const auto& expected = *subject.expectedMatchedAllocation;
        const auto& actual = *actualMatched;

        INFO("Flag: " << flag);
        INFO("Subject: " << subject.subjectKey);
        INFO("Matched allocation key: " << expected.key);
        INFO("Expected code: " << expected.allocationEvaluationCode);
        INFO("Actual code: " << allocationEvaluationCodeToString(actual.allocationEvaluationCode));

        REQUIRE(actual.key == expected.key);
        auto expectedCodeOpt = stringToAllocationEvaluationCode(expected.allocationEvaluationCode);
        REQUIRE(expectedCodeOpt.has_value());
        REQUIRE(actual.allocationEvaluationCode == *expectedCodeOpt);
        REQUIRE(actual.orderPosition == expected.orderPosition);
    } else {
        REQUIRE(!actualMatched.has_value());
    }

    // Validate unmatched allocations
    REQUIRE(actualUnmatched.size() == subject.expectedUnmatchedAllocations.size());
    for (size_t i = 0; i < subject.expectedUnmatchedAllocations.size(); i++) {
        const auto& expected = subject.expectedUnmatchedAllocations[i];
        // Find matching allocation by key
        auto it = std::find_if(actualUnmatched.begin(), actualUnmatched.end(),
            [&expected](const AllocationEvaluationDetails& alloc) {
                return alloc.key == expected.key;
            });

        REQUIRE(it != actualUnmatched.end());
        const auto& actual = *it;

        INFO("Flag: " << flag);
        INFO("Subject: " << subject.subjectKey);
        INFO("Unmatched allocation key: " << expected.key);
        INFO("Expected code: " << expected.allocationEvaluationCode);
        INFO("Actual code: " << allocationEvaluationCodeToString(actual.allocationEvaluationCode));

        auto expectedCodeOpt = stringToAllocationEvaluationCode(expected.allocationEvaluationCode);
        REQUIRE(expectedCodeOpt.has_value());
        REQUIRE(actual.allocationEvaluationCode == *expectedCodeOpt);
        REQUIRE(actual.orderPosition == expected.orderPosition);
    }

    // Validate unevaluated allocations
    REQUIRE(actualUnevaluated.size() == subject.expectedUnevaluatedAllocations.size());
    for (size_t i = 0; i < subject.expectedUnevaluatedAllocations.size(); i++) {
        const auto& expected = subject.expectedUnevaluatedAllocations[i];
        // Find matching allocation by key
        auto it = std::find_if(actualUnevaluated.begin(), actualUnevaluated.end(),
            [&expected](const AllocationEvaluationDetails& alloc) {
                return alloc.key == expected.key;
            });

        REQUIRE(it != actualUnevaluated.end());
        const auto& actual = *it;

        INFO("Flag: " << flag);
        INFO("Subject: " << subject.subjectKey);
        INFO("Unevaluated allocation key: " << expected.key);
        INFO("Expected code: " << expected.allocationEvaluationCode);
        INFO("Actual code: " << allocationEvaluationCodeToString(actual.allocationEvaluationCode));

        auto expectedCodeOpt = stringToAllocationEvaluationCode(expected.allocationEvaluationCode);
        REQUIRE(expectedCodeOpt.has_value());
        REQUIRE(actual.allocationEvaluationCode == *expectedCodeOpt);
        REQUIRE(actual.orderPosition == expected.orderPosition);
    }
}

TEST_CASE("UFC Test Cases - Flag Evaluation Details", "[ufc][evaluation-details]") {
    // Load flags configuration
    std::string flagsPath = "test/data/ufc/flags-v1.json";
    ConfigResponse configResponse;

    REQUIRE_NOTHROW(configResponse = loadFlagsConfiguration(flagsPath));

    // Create configuration
    Configuration config(configResponse);
    config.precompute();

    // Create client with configuration
    ConfigurationStore configStore;
    configStore.setConfiguration(config);

    EppoClient client(configStore, nullptr, nullptr);

    // Enable graceful failure mode to get evaluation details even on errors
    client.setIsGracefulFailureMode(true);

    // Load all test cases
    std::string testCasesDir = "test/data/ufc/tests";
    std::vector<EvaluationDetailsTestCase> testCases;

    REQUIRE_NOTHROW(testCases = loadAllTestCases(testCasesDir));
    REQUIRE(testCases.size() > 0);

    // Run through each test case
    for (const auto& testCase : testCases) {
        DYNAMIC_SECTION("Test case: " << testCase.filename << " - Flag: " << testCase.flag) {
            // Run through each subject in the test case
            for (size_t i = 0; i < testCase.subjects.size(); i++) {
                const auto& subject = testCase.subjects[i];

                DYNAMIC_SECTION("Subject " << i << ": " << subject.subjectKey) {
                    auto expectedCodeOpt = stringToFlagEvaluationCode(subject.expectedFlagEvaluationCode);
                    if (!expectedCodeOpt.has_value()) {
                        FAIL("Unknown flag evaluation code: " << subject.expectedFlagEvaluationCode);
                    }
                    FlagEvaluationCode expectedCode = *expectedCodeOpt;
                    FlagEvaluationCode actualCode;

                    // Get assignment details based on variation type
                    switch (testCase.variationType) {
                        case VariationType::BOOLEAN: {
                            bool defaultVal = testCase.defaultValue.get<bool>();
                            auto result = client.getBooleanAssignmentDetails(
                                testCase.flag,
                                subject.subjectKey,
                                subject.subjectAttributes,
                                defaultVal
                            );

                            // Verify flagEvaluationCode
                            REQUIRE(result.evaluationDetails.has_value());
                            REQUIRE(result.evaluationDetails->flagEvaluationCode.has_value());
                            actualCode = *result.evaluationDetails->flagEvaluationCode;

                            INFO("Flag: " << testCase.flag);
                            INFO("Subject: " << subject.subjectKey);
                            INFO("Expected code: " << subject.expectedFlagEvaluationCode);
                            INFO("Actual code: " << flagEvaluationCodeToString(actualCode));

                            REQUIRE(actualCode == expectedCode);

                            // Verify allocation evaluation codes
                            validateAllocationEvaluationCodes(*result.evaluationDetails, subject, testCase.flag);
                            break;
                        }

                        case VariationType::STRING: {
                            std::string defaultVal = testCase.defaultValue.get<std::string>();
                            auto result = client.getStringAssignmentDetails(
                                testCase.flag,
                                subject.subjectKey,
                                subject.subjectAttributes,
                                defaultVal
                            );

                            // Verify flagEvaluationCode
                            REQUIRE(result.evaluationDetails.has_value());
                            REQUIRE(result.evaluationDetails->flagEvaluationCode.has_value());
                            actualCode = *result.evaluationDetails->flagEvaluationCode;

                            INFO("Flag: " << testCase.flag);
                            INFO("Subject: " << subject.subjectKey);
                            INFO("Expected code: " << subject.expectedFlagEvaluationCode);
                            INFO("Actual code: " << flagEvaluationCodeToString(actualCode));

                            REQUIRE(actualCode == expectedCode);

                            // Verify allocation evaluation codes
                            validateAllocationEvaluationCodes(*result.evaluationDetails, subject, testCase.flag);
                            break;
                        }

                        case VariationType::INTEGER: {
                            int64_t defaultVal = testCase.defaultValue.get<int64_t>();
                            auto result = client.getIntegerAssignmentDetails(
                                testCase.flag,
                                subject.subjectKey,
                                subject.subjectAttributes,
                                defaultVal
                            );

                            // Verify flagEvaluationCode
                            REQUIRE(result.evaluationDetails.has_value());
                            REQUIRE(result.evaluationDetails->flagEvaluationCode.has_value());
                            actualCode = *result.evaluationDetails->flagEvaluationCode;

                            INFO("Flag: " << testCase.flag);
                            INFO("Subject: " << subject.subjectKey);
                            INFO("Expected code: " << subject.expectedFlagEvaluationCode);
                            INFO("Actual code: " << flagEvaluationCodeToString(actualCode));

                            REQUIRE(actualCode == expectedCode);

                            // Verify allocation evaluation codes
                            validateAllocationEvaluationCodes(*result.evaluationDetails, subject, testCase.flag);
                            break;
                        }

                        case VariationType::NUMERIC: {
                            double defaultVal = testCase.defaultValue.get<double>();
                            auto result = client.getNumericAssignmentDetails(
                                testCase.flag,
                                subject.subjectKey,
                                subject.subjectAttributes,
                                defaultVal
                            );

                            // Verify flagEvaluationCode
                            REQUIRE(result.evaluationDetails.has_value());
                            REQUIRE(result.evaluationDetails->flagEvaluationCode.has_value());
                            actualCode = *result.evaluationDetails->flagEvaluationCode;

                            INFO("Flag: " << testCase.flag);
                            INFO("Subject: " << subject.subjectKey);
                            INFO("Expected code: " << subject.expectedFlagEvaluationCode);
                            INFO("Actual code: " << flagEvaluationCodeToString(actualCode));

                            REQUIRE(actualCode == expectedCode);

                            // Verify allocation evaluation codes
                            validateAllocationEvaluationCodes(*result.evaluationDetails, subject, testCase.flag);
                            break;
                        }

                        case VariationType::JSON: {
                            json defaultVal = testCase.defaultValue;
                            auto result = client.getJsonAssignmentDetails(
                                testCase.flag,
                                subject.subjectKey,
                                subject.subjectAttributes,
                                defaultVal
                            );

                            // Verify flagEvaluationCode
                            REQUIRE(result.evaluationDetails.has_value());
                            REQUIRE(result.evaluationDetails->flagEvaluationCode.has_value());
                            actualCode = *result.evaluationDetails->flagEvaluationCode;

                            INFO("Flag: " << testCase.flag);
                            INFO("Subject: " << subject.subjectKey);
                            INFO("Expected code: " << subject.expectedFlagEvaluationCode);
                            INFO("Actual code: " << flagEvaluationCodeToString(actualCode));

                            REQUIRE(actualCode == expectedCode);

                            // Verify allocation evaluation codes
                            validateAllocationEvaluationCodes(*result.evaluationDetails, subject, testCase.flag);
                            break;
                        }
                    }
                }
            }
        }
    }
}

// Test to verify we can load test cases with evaluation details
TEST_CASE("Load test cases with evaluation details", "[ufc][evaluation-details][loading]") {
    std::string testCasesDir = "test/data/ufc/tests";

    SECTION("Test cases directory exists and contains test files with evaluation details") {
        std::vector<EvaluationDetailsTestCase> testCases;
        REQUIRE_NOTHROW(testCases = loadAllTestCases(testCasesDir));

        // Verify we have some test cases
        REQUIRE(testCases.size() > 0);

        // Print out loaded test cases for debugging
        std::cout << "Loaded " << testCases.size() << " test cases with evaluation details:" << std::endl;
        for (const auto& tc : testCases) {
            std::cout << "  - " << tc.filename << " (flag: " << tc.flag
                      << ", subjects with details: " << tc.subjects.size() << ")" << std::endl;
        }

        // Verify that all subjects have evaluation codes
        for (const auto& tc : testCases) {
            for (const auto& subject : tc.subjects) {
                REQUIRE(!subject.expectedFlagEvaluationCode.empty());
            }
        }
    }
}

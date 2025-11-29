#include <catch_amalgamated.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include "../src/client.hpp"
#include "../src/config_response.hpp"

using namespace eppoclient;
using json = nlohmann::json;
namespace fs = std::filesystem;

// Structure to hold test subject data
struct TestSubject {
    std::string subjectKey;
    Attributes subjectAttributes;
    json expectedAssignment;  // Can be bool, int, double, string, or json
};

// Structure to hold test case data
struct TestCase {
    std::string flag;
    VariationType variationType;
    json defaultValue;
    std::vector<TestSubject> subjects;
    std::string filename;  // For better error messages
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
            // Skip null values or handle them specially
            continue;
        }
    }

    return attributes;
}


// Helper function to load a single test case from JSON file
TestCase loadTestCase(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open test case file: " + filepath);
    }

    json j;
    file >> j;

    TestCase testCase;
    testCase.flag = j["flag"].get_ref<const std::string&>();

    // Parse variationType safely
    std::string parseError;
    auto varType = internal::parseVariationType(j["variationType"], parseError);
    if (!varType) {
        throw std::runtime_error("Invalid variationType in test case: " + parseError);
    }
    testCase.variationType = *varType;

    testCase.defaultValue = j["defaultValue"];
    testCase.filename = fs::path(filepath).filename().string();

    // Parse subjects
    for (const auto& subjectJson : j["subjects"]) {
        TestSubject subject;
        subject.subjectKey = subjectJson["subjectKey"].get_ref<const std::string&>();
        subject.subjectAttributes = parseAttributes(subjectJson["subjectAttributes"]);
        subject.expectedAssignment = subjectJson["assignment"];
        testCase.subjects.push_back(subject);
    }

    return testCase;
}

// Helper function to load all test cases from directory
std::vector<TestCase> loadAllTestCases(const std::string& directory) {
    std::vector<TestCase> testCases;

    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            std::string filename = entry.path().filename().string();
            // Skip files that don't start with "test-"
            if (filename.find("test-") == 0) {
                try {
                    testCases.push_back(loadTestCase(entry.path().string()));
                } catch (const std::exception& e) {
                    std::cerr << "Warning: Failed to load test case " << filename << ": "
                              << e.what() << std::endl;
                }
            }
        }
    }

    return testCases;
}

// Helper function to compare expected and actual values
bool compareValues(
    const json& expected,
    const std::optional<std::variant<std::string, int64_t, double, bool, json>>& actual,
    VariationType type) {
    if (!actual.has_value()) {
        return false;
    }

    switch (type) {
        case VariationType::BOOLEAN:
            if (expected.is_boolean() && std::holds_alternative<bool>(*actual)) {
                return expected.get_ref<const json::boolean_t&>() == std::get<bool>(*actual);
            }
            break;

        case VariationType::STRING:
            if (expected.is_string() && std::holds_alternative<std::string>(*actual)) {
                return expected.get_ref<const std::string&>() == std::get<std::string>(*actual);
            }
            break;

        case VariationType::INTEGER:
            if (expected.is_number_integer() && std::holds_alternative<int64_t>(*actual)) {
                return expected.get_ref<const json::number_integer_t&>() ==
                       std::get<int64_t>(*actual);
            }
            break;

        case VariationType::NUMERIC:
            if (expected.is_number() && std::holds_alternative<double>(*actual)) {
                double expectedVal = expected.get<double>();
                double actualVal = std::get<double>(*actual);
                // Use approximate comparison for floating point
                return std::abs(expectedVal - actualVal) < 1e-9;
            }
            break;

        case VariationType::JSON:
            if (expected.is_object() && std::holds_alternative<json>(*actual)) {
                return expected == std::get<json>(*actual);
            }
            break;
    }

    return false;
}

TEST_CASE("UFC Test Cases - Flag Assignments", "[ufc][flags]") {
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

    // Load all test cases
    std::string testCasesDir = "test/data/ufc/tests";
    std::vector<TestCase> testCases;

    REQUIRE_NOTHROW(testCases = loadAllTestCases(testCasesDir));
    REQUIRE(testCases.size() > 0);

    // Run through each test case
    for (const auto& testCase : testCases) {
        DYNAMIC_SECTION("Test case: " << testCase.filename << " - Flag: " << testCase.flag) {
            // Run through each subject in the test case
            for (size_t i = 0; i < testCase.subjects.size(); i++) {
                const auto& subject = testCase.subjects[i];

                DYNAMIC_SECTION("Subject " << i << ": " << subject.subjectKey) {
                    std::optional<std::variant<std::string, int64_t, double, bool, json>>
                        assignment;

                    // Get assignment based on variation type
                    switch (testCase.variationType) {
                        case VariationType::BOOLEAN: {
                            bool defaultVal = testCase.defaultValue.get<bool>();
                            bool result =
                                client.getBoolAssignment(testCase.flag, subject.subjectKey,
                                                         subject.subjectAttributes, defaultVal);
                            assignment = result;
                            break;
                        }

                        case VariationType::STRING: {
                            std::string defaultVal = testCase.defaultValue.get<std::string>();
                            std::string result =
                                client.getStringAssignment(testCase.flag, subject.subjectKey,
                                                           subject.subjectAttributes, defaultVal);
                            assignment = result;
                            break;
                        }

                        case VariationType::INTEGER: {
                            int64_t defaultVal = testCase.defaultValue.get<int64_t>();
                            int64_t result =
                                client.getIntegerAssignment(testCase.flag, subject.subjectKey,
                                                            subject.subjectAttributes, defaultVal);
                            assignment = result;
                            break;
                        }

                        case VariationType::NUMERIC: {
                            double defaultVal = testCase.defaultValue.get<double>();
                            double result =
                                client.getNumericAssignment(testCase.flag, subject.subjectKey,
                                                            subject.subjectAttributes, defaultVal);
                            assignment = result;
                            break;
                        }

                        case VariationType::JSON: {
                            json defaultVal = testCase.defaultValue;
                            json result =
                                client.getJSONAssignment(testCase.flag, subject.subjectKey,
                                                         subject.subjectAttributes, defaultVal);
                            assignment = result;
                            break;
                        }
                    }

                    // Verify the assignment matches expected value
                    bool matches = compareValues(subject.expectedAssignment, assignment,
                                                 testCase.variationType);

                    if (!matches) {
                        INFO("Flag: " << testCase.flag);
                        INFO("Subject: " << subject.subjectKey);
                        INFO("Expected: " << subject.expectedAssignment.dump());
                        if (assignment.has_value()) {
                            std::string actualStr;
                            std::visit(
                                [&actualStr](auto&& arg) {
                                    using T = std::decay_t<decltype(arg)>;
                                    if constexpr (std::is_same_v<T, std::string>) {
                                        actualStr = arg;
                                    } else if constexpr (std::is_same_v<T, bool>) {
                                        actualStr = arg ? "true" : "false";
                                    } else if constexpr (std::is_same_v<T, int64_t>) {
                                        actualStr = std::to_string(arg);
                                    } else if constexpr (std::is_same_v<T, double>) {
                                        actualStr = std::to_string(arg);
                                    } else if constexpr (std::is_same_v<T, json>) {
                                        actualStr = arg.dump();
                                    }
                                },
                                *assignment);
                            INFO("Actual: " << actualStr);
                        } else {
                            INFO("Actual: (no value)");
                        }
                    }

                    REQUIRE(matches);
                }
            }
        }
    }
}

// Additional test to verify we can load the flags configuration
TEST_CASE("Load flags configuration", "[ufc][config]") {
    std::string flagsPath = "test/data/ufc/flags-v1.json";

    SECTION("Flags file exists and can be parsed") {
        ConfigResponse response;
        REQUIRE_NOTHROW(response = loadFlagsConfiguration(flagsPath));

        // Verify we have some flags
        REQUIRE(response.flags.size() > 0);

        // Check for specific known flags
        CHECK(response.flags.count("kill-switch") > 0);
        CHECK(response.flags.count("numeric_flag") > 0);
        CHECK(response.flags.count("boolean-false-assignment") > 0);
    }
}

// Test to verify we can load all test cases
TEST_CASE("Load all test cases", "[ufc][testcases]") {
    std::string testCasesDir = "test/data/ufc/tests";

    SECTION("Test cases directory exists and contains test files") {
        std::vector<TestCase> testCases;
        REQUIRE_NOTHROW(testCases = loadAllTestCases(testCasesDir));

        // Verify we have some test cases
        REQUIRE(testCases.size() > 0);

        // Print out loaded test cases for debugging
        std::cout << "Loaded " << testCases.size() << " test cases:" << std::endl;
        for (const auto& tc : testCases) {
            std::cout << "  - " << tc.filename << " (flag: " << tc.flag
                      << ", subjects: " << tc.subjects.size() << ")" << std::endl;
        }
    }
}

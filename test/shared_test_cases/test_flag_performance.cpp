#include <algorithm>
#include <catch_amalgamated.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <numeric>
#include <vector>
#include "../src/client.hpp"
#include "../src/config_response.hpp"
#include "../src/rules.hpp"

using namespace eppoclient;
using json = nlohmann::json;
namespace fs = std::filesystem;

// Structure to hold test subject data
struct PerformanceTestSubject {
    std::string subjectKey;
    Attributes subjectAttributes;
};

// Structure to hold test case data
struct PerformanceTestCase {
    std::string flag;
    VariationType variationType;
    json defaultValue;
    std::vector<PerformanceTestSubject> subjects;
    std::string filename;
};

// Structure to hold timing result with context
struct TimingResult {
    double time_us;
    std::string flagKey;
    std::string subjectKey;
    std::string sourceFile;
    VariationType variationType;
};

// Helper function to load flags configuration from JSON file
static ConfigResponse loadFlagsConfiguration(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open flags configuration file: " + filepath);
    }

    // Read entire file content into a string
    std::string configJson((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    // Parse configuration using parseConfigResponse
    std::string error;
    ConfigResponse response = parseConfigResponse(configJson, error);

    if (!error.empty()) {
        throw std::runtime_error("Failed to parse configuration: " + error);
    }

    return response;
}

// Helper function to parse attributes from JSON
static Attributes parseAttributes(const json& attrJson) {
    Attributes attributes;
    if (!attrJson.is_object()) {
        return attributes;
    }

    for (auto it = attrJson.begin(); it != attrJson.end(); ++it) {
        std::string key = it.key();
        const auto& value = it.value();

        try {
            if (value.is_string()) {
                attributes[key] = value.get<std::string>();
            } else if (value.is_number_integer()) {
                attributes[key] = value.get<int64_t>();
            } else if (value.is_number_float()) {
                attributes[key] = value.get<double>();
            } else if (value.is_boolean()) {
                attributes[key] = value.get<bool>();
            }
        } catch (const std::exception&) {
            // Skip attributes that can't be parsed
        }
    }

    return attributes;
}

// Helper function to load a single test case from JSON file
static PerformanceTestCase loadTestCase(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open test case file: " + filepath);
    }

    json j;
    file >> j;

    PerformanceTestCase testCase;
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
    if (j.contains("subjects") && j["subjects"].is_array()) {
        for (const auto& subjectJson : j["subjects"]) {
            if (!subjectJson.contains("subjectKey") || !subjectJson.contains("subjectAttributes")) {
                continue;
            }

            std::string subjectKey = subjectJson["subjectKey"].get_ref<const std::string&>();

            if (subjectKey.empty()) {
                continue;
            }

            Attributes subjectAttributes = parseAttributes(subjectJson["subjectAttributes"]);
            testCase.subjects.push_back(
                PerformanceTestSubject{std::move(subjectKey), std::move(subjectAttributes)});
        }
    }

    return testCase;
}

// Helper function to load all test cases from directory
static std::vector<PerformanceTestCase> loadAllTestCases(const std::string& directory,
                                                         size_t maxTestCases = 20) {
    std::vector<PerformanceTestCase> testCases;

    try {
        if (!fs::exists(directory) || !fs::is_directory(directory)) {
            return testCases;
        }

        for (const auto& entry : fs::directory_iterator(directory)) {
            if (testCases.size() >= maxTestCases) {
                std::cout << "  Reached max test cases limit (" << maxTestCases << ")\n";
                break;
            }

            if (!entry.is_regular_file() || entry.path().extension() != ".json") {
                continue;
            }

            std::string filename = entry.path().filename().string();
            if (filename.find("test-") != 0) {
                continue;
            }

            try {
                std::cout << "  Loading " << filename << "..." << std::endl;
                PerformanceTestCase testCase = loadTestCase(entry.path().string());
                std::cout << "    Flag: " << testCase.flag
                          << ", Subjects: " << testCase.subjects.size() << std::endl;
                if (!testCase.subjects.empty()) {
                    testCases.push_back(testCase);
                }
            } catch (const std::exception& e) {
                // Skip files that can't be loaded
                std::cerr << "Warning: Failed to load test case " << filename << ": " << e.what()
                          << std::endl;
            }
        }
    } catch (const std::exception& e) {
        // Return whatever test cases we've collected so far
        std::cerr << "Exception in loadAllTestCases: " << e.what() << std::endl;
    }

    return testCases;
}

TEST_CASE("Performance - Flag Evaluation Timing", "[performance][flags]") {
    std::cout << "Starting test...\n";

    // Load flags configuration
    std::string flagsPath = "test/data/ufc/flags-v1.json";
    std::cout << "Loading flags configuration...\n";
    ConfigResponse configResponse;
    try {
        configResponse = loadFlagsConfiguration(flagsPath);
        std::cout << "Configuration loaded successfully\n";
    } catch (const std::exception& e) {
        std::cerr << "Failed to load configuration: " << e.what() << std::endl;
        throw;
    }

    // Create configuration
    std::cout << "Creating configuration...\n";
    Configuration config(configResponse);


    // Create client with configuration
    std::cout << "Creating client...\n";
    auto configStore = std::make_shared<ConfigurationStore>(config);
    EppoClient client(configStore, nullptr, nullptr);
    std::cout << "Client created\n";

    // Load all test cases
    std::string testCasesDir = "test/data/ufc/tests";
    std::cout << "Loading test cases...\n";
    std::vector<PerformanceTestCase> testCases;
    try {
        testCases = loadAllTestCases(testCasesDir);
        std::cout << "Test cases loaded: " << testCases.size() << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Failed to load test cases: " << e.what() << std::endl;
        throw;
    }

    REQUIRE(testCases.size() > 0);

    std::cout << "\nLoaded " << testCases.size() << " test cases for performance testing\n";

    // Count total subjects
    size_t totalSubjects = 0;
    for (const auto& testCase : testCases) {
        totalSubjects += testCase.subjects.size();
    }
    std::cout << "Total subjects across all test cases: " << totalSubjects << "\n";

    // Scale iterations based on test data availability
    // Use at least 100 iterations per subject, but cap at reasonable limits
    const int MIN_ITERATIONS = 100;
    const int MAX_ITERATIONS = 10000;
    const int ITERATIONS_PER_SUBJECT = 100;

    int iterations = std::min(
        MAX_ITERATIONS,
        std::max(MIN_ITERATIONS, static_cast<int>(totalSubjects * ITERATIONS_PER_SUBJECT)));

    std::cout << "Running " << iterations << " iterations\n";

    std::vector<TimingResult> allTimingResults;
    try {
        allTimingResults.reserve(iterations);
    } catch (const std::exception& e) {
        std::cerr << "Failed to reserve vector: " << e.what() << std::endl;
        throw;
    }

    // Warmup - run through all test cases once
    std::cout << "Starting warmup...\n";
    for (const auto& testCase : testCases) {
        if (testCase.subjects.empty())
            continue;

        for (const auto& subject : testCase.subjects) {
            try {
                switch (testCase.variationType) {
                    case VariationType::BOOLEAN: {
                        bool defaultVal = testCase.defaultValue.get<bool>();
                        client.getBoolAssignment(testCase.flag, subject.subjectKey,
                                                 subject.subjectAttributes, defaultVal);
                        break;
                    }
                    case VariationType::STRING: {
                        std::string defaultVal = testCase.defaultValue.get<std::string>();
                        client.getStringAssignment(testCase.flag, subject.subjectKey,
                                                   subject.subjectAttributes, defaultVal);
                        break;
                    }
                    case VariationType::INTEGER: {
                        int64_t defaultVal = testCase.defaultValue.get<int64_t>();
                        client.getIntegerAssignment(testCase.flag, subject.subjectKey,
                                                    subject.subjectAttributes, defaultVal);
                        break;
                    }
                    case VariationType::NUMERIC: {
                        double defaultVal = testCase.defaultValue.get<double>();
                        client.getNumericAssignment(testCase.flag, subject.subjectKey,
                                                    subject.subjectAttributes, defaultVal);
                        break;
                    }
                    case VariationType::JSON: {
                        json defaultVal = testCase.defaultValue;
                        client.getJSONAssignment(testCase.flag, subject.subjectKey,
                                                 subject.subjectAttributes, defaultVal);
                        break;
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "Warmup error for " << testCase.flag << ": " << e.what() << std::endl;
            }
        }
    }
    std::cout << "Warmup complete\n";

    // Run performance test - iterate through all test cases
    std::cout << "Starting performance test...\n";
    for (int i = 0; i < iterations; i++) {
        const auto& testCase = testCases[static_cast<size_t>(i) % testCases.size()];

        if (testCase.subjects.empty()) {
            std::cerr << "Test case " << testCase.filename << " has no subjects, skipping\n";
            continue;
        }

        const auto& subject = testCase.subjects[static_cast<size_t>(i) % testCase.subjects.size()];

        try {
            auto start = std::chrono::high_resolution_clock::now();

            switch (testCase.variationType) {
                case VariationType::BOOLEAN: {
                    bool defaultVal = testCase.defaultValue.get<bool>();
                    client.getBoolAssignment(testCase.flag, subject.subjectKey,
                                             subject.subjectAttributes, defaultVal);
                    break;
                }
                case VariationType::STRING: {
                    std::string defaultVal = testCase.defaultValue.get<std::string>();
                    client.getStringAssignment(testCase.flag, subject.subjectKey,
                                               subject.subjectAttributes, defaultVal);
                    break;
                }
                case VariationType::INTEGER: {
                    int64_t defaultVal = testCase.defaultValue.get<int64_t>();
                    client.getIntegerAssignment(testCase.flag, subject.subjectKey,
                                                subject.subjectAttributes, defaultVal);
                    break;
                }
                case VariationType::NUMERIC: {
                    double defaultVal = testCase.defaultValue.get<double>();
                    client.getNumericAssignment(testCase.flag, subject.subjectKey,
                                                subject.subjectAttributes, defaultVal);
                    break;
                }
                case VariationType::JSON: {
                    json defaultVal = testCase.defaultValue;
                    client.getJSONAssignment(testCase.flag, subject.subjectKey,
                                             subject.subjectAttributes, defaultVal);
                    break;
                }
            }

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

            TimingResult result;
            result.time_us = duration.count() / 1000.0;
            result.flagKey = testCase.flag;
            result.subjectKey = subject.subjectKey;
            result.sourceFile = testCase.filename;
            result.variationType = testCase.variationType;
            allTimingResults.push_back(result);
        } catch (const std::exception& e) {
            std::cerr << "Error during iteration " << i << " for " << testCase.flag << ": "
                      << e.what() << std::endl;
        }
    }
    std::cout << "Performance test complete. Collected " << allTimingResults.size() << " results\n";

    // Group results by variation type
    std::map<VariationType, std::vector<TimingResult>> resultsByType;
    for (const auto& result : allTimingResults) {
        resultsByType[result.variationType].push_back(result);
    }

    // Output statistics for each variation type
    for (const auto& [varType, results] : resultsByType) {
        if (results.empty()) {
            continue;
        }

        // Find min and max results
        auto minResult = *std::min_element(
            results.begin(), results.end(),
            [](const TimingResult& a, const TimingResult& b) { return a.time_us < b.time_us; });
        auto maxResult = *std::max_element(
            results.begin(), results.end(),
            [](const TimingResult& a, const TimingResult& b) { return a.time_us < b.time_us; });

        // Calculate statistics
        std::vector<double> times;
        times.reserve(results.size());
        for (const auto& r : results) {
            times.push_back(r.time_us);
        }
        std::sort(times.begin(), times.end());

        double avg_time = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
        double median_time = times[times.size() / 2];

        // Calculate percentiles safely
        size_t p95_index = std::min(static_cast<size_t>(times.size() * 0.95), times.size() - 1);
        size_t p99_index = std::min(static_cast<size_t>(times.size() * 0.99), times.size() - 1);
        double p95_time = times[p95_index];
        double p99_time = times[p99_index];

        // Output statistics
        std::cout << "\n=== " << variationTypeToString(varType)
                  << " Flag Evaluation Performance ===\n";
        std::cout << std::fixed << std::setprecision(3);
        std::cout << "Evaluations: " << results.size() << "\n";
        std::cout << "Min:    " << minResult.time_us << " μs"
                  << " (flag: " << minResult.flagKey << ", subject: " << minResult.subjectKey
                  << ", file: " << minResult.sourceFile << ")\n";
        std::cout << "Max:    " << maxResult.time_us << " μs"
                  << " (flag: " << maxResult.flagKey << ", subject: " << maxResult.subjectKey
                  << ", file: " << maxResult.sourceFile << ")\n";
        std::cout << "Avg:    " << avg_time << " μs\n";
        std::cout << "Median: " << median_time << " μs\n";
        std::cout << "P95:    " << p95_time << " μs\n";
        std::cout << "P99:    " << p99_time << " μs\n";
        std::cout << "===========================================\n";
    }

    // Test passes - this is just performance measurement
    REQUIRE(true);
}

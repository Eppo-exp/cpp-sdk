#include <catch_amalgamated.hpp>
#include <chrono>
#include <ctime>
#include "../src/time_utils.hpp"

using namespace eppoclient;

TEST_CASE("parseISOTimestamp - valid timestamp without milliseconds", "[time_utils]") {
    std::string timestamp = "2024-06-09T14:23:11";
    std::string error;
    auto result = parseISOTimestamp(timestamp, error);

    // Convert back to time_t to check the values
    auto time_t_result = std::chrono::system_clock::to_time_t(result);
    std::tm* tm = std::gmtime(&time_t_result);

    REQUIRE(tm->tm_year + 1900 == 2024);
    REQUIRE(tm->tm_mon + 1 == 6);
    REQUIRE(tm->tm_mday == 9);
    REQUIRE(tm->tm_hour == 14);
    REQUIRE(tm->tm_min == 23);
    REQUIRE(tm->tm_sec == 11);
    REQUIRE(error.empty());
}

TEST_CASE("parseISOTimestamp - valid timestamp with milliseconds", "[time_utils]") {
    std::string timestamp = "2024-06-09T14:23:11.123";
    std::string error;
    auto result = parseISOTimestamp(timestamp, error);

    // Convert back to time_t to check the values
    auto time_t_result = std::chrono::system_clock::to_time_t(result);
    std::tm* tm = std::gmtime(&time_t_result);

    REQUIRE(tm->tm_year + 1900 == 2024);
    REQUIRE(tm->tm_mon + 1 == 6);
    REQUIRE(tm->tm_mday == 9);
    REQUIRE(tm->tm_hour == 14);
    REQUIRE(tm->tm_min == 23);
    REQUIRE(tm->tm_sec == 11);

    // Check milliseconds by calculating the difference
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(result.time_since_epoch());
    auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(result.time_since_epoch()) - seconds;
    REQUIRE(milliseconds.count() == 123);
    REQUIRE(error.empty());
}

TEST_CASE("parseISOTimestamp - timestamp with single digit milliseconds", "[time_utils]") {
    std::string timestamp = "2024-06-09T14:23:11.5";
    std::string error;
    auto result = parseISOTimestamp(timestamp, error);

    // Should pad to 500 milliseconds
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(result.time_since_epoch());
    auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(result.time_since_epoch()) - seconds;
    REQUIRE(milliseconds.count() == 500);
    REQUIRE(error.empty());
}

TEST_CASE("parseISOTimestamp - timestamp with two digit milliseconds", "[time_utils]") {
    std::string timestamp = "2024-06-09T14:23:11.45";
    std::string error;
    auto result = parseISOTimestamp(timestamp, error);

    // Should pad to 450 milliseconds
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(result.time_since_epoch());
    auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(result.time_since_epoch()) - seconds;
    REQUIRE(milliseconds.count() == 450);
    REQUIRE(error.empty());
}

TEST_CASE("parseISOTimestamp - timestamp with microseconds (ignores extra precision)",
          "[time_utils]") {
    std::string timestamp = "2024-06-09T14:23:11.123456";
    std::string error;
    auto result = parseISOTimestamp(timestamp, error);

    // Should only use first 3 digits (123 milliseconds)
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(result.time_since_epoch());
    auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(result.time_since_epoch()) - seconds;
    REQUIRE(milliseconds.count() == 123);
    REQUIRE(error.empty());
}

TEST_CASE("parseISOTimestamp - timestamp with Z suffix", "[time_utils]") {
    std::string timestamp = "2024-06-09T14:23:11Z";
    std::string error;
    auto result = parseISOTimestamp(timestamp, error);

    // Convert back to time_t to check the values
    auto time_t_result = std::chrono::system_clock::to_time_t(result);
    std::tm* tm = std::gmtime(&time_t_result);

    REQUIRE(tm->tm_year + 1900 == 2024);
    REQUIRE(tm->tm_mon + 1 == 6);
    REQUIRE(tm->tm_mday == 9);
    REQUIRE(tm->tm_hour == 14);
    REQUIRE(tm->tm_min == 23);
    REQUIRE(tm->tm_sec == 11);
    REQUIRE(error.empty());
}

TEST_CASE("parseISOTimestamp - invalid timestamp returns default time_point", "[time_utils]") {
    std::string invalidTimestamp = "not-a-timestamp";
    std::string error;
    auto result = parseISOTimestamp(invalidTimestamp, error);

    // Should return default-constructed time_point
    REQUIRE(result == std::chrono::system_clock::time_point());
    REQUIRE(!error.empty());
}

TEST_CASE("parseISOTimestamp - empty string returns default time_point", "[time_utils]") {
    std::string emptyTimestamp = "";
    std::string error;
    auto result = parseISOTimestamp(emptyTimestamp, error);

    // Should return default-constructed time_point
    REQUIRE(result == std::chrono::system_clock::time_point());
    REQUIRE(!error.empty());
}

TEST_CASE("parseISOTimestamp - malformed date returns default time_point", "[time_utils]") {
    std::string malformedTimestamp = "2024-13-40T25:99:99";
    std::string error;
    auto result = parseISOTimestamp(malformedTimestamp, error);

    // Should return default-constructed time_point
    REQUIRE(result == std::chrono::system_clock::time_point());
    REQUIRE(!error.empty());
}

TEST_CASE("formatISOTimestamp - formats time_point to ISO 8601 string", "[time_utils]") {
    // Create a specific time point: 2024-06-09 14:23:11 UTC
    std::tm tm = {};
    tm.tm_year = 2024 - 1900;
    tm.tm_mon = 6 - 1;
    tm.tm_mday = 9;
    tm.tm_hour = 14;
    tm.tm_min = 23;
    tm.tm_sec = 11;

    auto time_c = std::mktime(&tm);
    auto tp = std::chrono::system_clock::from_time_t(time_c);

    std::string result = formatISOTimestamp(tp);

    // The result should be in ISO 8601 format with milliseconds and Z suffix
    // Note: The exact time may vary based on timezone conversion
    REQUIRE(result.size() == 24);  // YYYY-MM-DDTHH:MM:SS.sssZ is 24 characters
    REQUIRE(result[4] == '-');
    REQUIRE(result[7] == '-');
    REQUIRE(result[10] == 'T');
    REQUIRE(result[13] == ':');
    REQUIRE(result[16] == ':');
    REQUIRE(result[19] == '.');
    REQUIRE(result[23] == 'Z');
}

TEST_CASE("formatISOTimestamp - default time_point formats to epoch", "[time_utils]") {
    auto tp = std::chrono::system_clock::time_point();
    std::string result = formatISOTimestamp(tp);

    // Should format the epoch time with milliseconds (1970-01-01T00:00:00.000Z)
    REQUIRE(result == "1970-01-01T00:00:00.000Z");
}

TEST_CASE("parseISOTimestamp and formatISOTimestamp - round trip without milliseconds",
          "[time_utils]") {
    std::string original = "2024-06-09T14:23:11Z";

    // Parse and format
    std::string error;
    auto parsed = parseISOTimestamp(original, error);
    std::string formatted = formatISOTimestamp(parsed);

    // The formatted result will include .000 for milliseconds even if original didn't have them
    REQUIRE(formatted == "2024-06-09T14:23:11.000Z");
    REQUIRE(error.empty());
}

TEST_CASE("parseISOTimestamp and formatISOTimestamp - round trip preserves milliseconds",
          "[time_utils]") {
    std::string original = "2024-06-09T14:23:11.123";

    // Parse and format
    std::string error;
    auto parsed = parseISOTimestamp(original, error);
    std::string formatted = formatISOTimestamp(parsed);

    REQUIRE(formatted == "2024-06-09T14:23:11.123Z");

    // But the milliseconds should be preserved in the time_point
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(parsed.time_since_epoch());
    auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(parsed.time_since_epoch()) - seconds;
    REQUIRE(milliseconds.count() == 123);
    REQUIRE(error.empty());
}

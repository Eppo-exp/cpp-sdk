#ifndef EPPOCLIENT_TIME_UTILS_HPP_
#define EPPOCLIENT_TIME_UTILS_HPP_

#include <chrono>
#include <string>

namespace eppoclient {

/**
 * Parse an ISO 8601 timestamp string into a system_clock::time_point.
 *
 * Supports ISO8601 timestamps with optional millisecond precision.
 * Examples: "2024-06-09T14:23:11", "2024-06-09T14:23:11.123"
 *
 * @param timestamp The ISO 8601 formatted timestamp string
 * @param errorValue Optional value to return when time_c == -1 (defaults to epoch)
 * @return A time_point representing the parsed timestamp, or errorValue if parsing fails
 */
std::chrono::system_clock::time_point parseISOTimestamp(
    const std::string& timestamp,
    std::chrono::system_clock::time_point errorValue = std::chrono::system_clock::time_point());

/**
 * Parse an ISO 8601 timestamp string for configuration endAt field.
 *
 * Returns time_point::max() on parsing failures, which represents "no end date".
 *
 * @param timestamp The ISO 8601 formatted timestamp string
 * @return A time_point representing the parsed timestamp, or time_point::max() on failure
 */
std::chrono::system_clock::time_point parseISOTimestampForConfigEndTime(
    const std::string& timestamp);

/**
 * Parse an ISO 8601 timestamp string for configuration startAt field.
 *
 * Returns epoch (default time_point) on parsing failures.
 *
 * @param timestamp The ISO 8601 formatted timestamp string
 * @return A time_point representing the parsed timestamp, or epoch on failure
 */
std::chrono::system_clock::time_point parseISOTimestampForConfigStartTime(
    const std::string& timestamp);

/**
 * Format a system_clock::time_point into an ISO 8601 timestamp string.
 *
 * Outputs format: "YYYY-MM-DDTHH:MM:SS.sssZ" (UTC with milliseconds)
 *
 * @param tp The time_point to format
 * @return An ISO 8601 formatted timestamp string with milliseconds and 'Z' suffix indicating UTC
 */
std::string formatISOTimestamp(const std::chrono::system_clock::time_point& tp);

/**
 * Format a system_clock::time_point for configuration endAt field.
 *
 * Handles the special case of time_point::max() which represents "no end date"
 * and formats it as "9999-12-31T00:00:00.000Z" for configuration consistency.
 *
 * @param tp The time_point to format
 * @return An ISO 8601 formatted timestamp string, or "9999-12-31T00:00:00.000Z" for max
 */
std::string formatISOTimestampForConfigEndTime(const std::chrono::system_clock::time_point& tp);

}  // namespace eppoclient

#endif  // EPPOCLIENT_TIME_UTILS_HPP_

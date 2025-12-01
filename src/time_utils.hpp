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
 * @param fallbackTime The time_point to return if parsing fails
 * @return A time_point representing the parsed timestamp, or the fallback time_point if parsing
 * fails time_point if parsing fails
 */
std::chrono::system_clock::time_point parseISOTimestamp(
    const std::string& timestamp,
    std::chrono::system_clock::time_point fallbackTime = std::chrono::system_clock::time_point());

/**
 * Format a system_clock::time_point into an ISO 8601 timestamp string.
 *
 * Outputs format: "YYYY-MM-DDTHH:MM:SS.sssZ" (UTC with milliseconds)
 *
 * @param tp The time_point to format
 * @return An ISO 8601 formatted timestamp string with milliseconds and 'Z' suffix indicating UTC
 */
std::string formatISOTimestamp(const std::chrono::system_clock::time_point& tp);

}  // namespace eppoclient

#endif  // EPPOCLIENT_TIME_UTILS_HPP_

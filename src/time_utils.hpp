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
 * Handles edge cases including date underflow and overflow scenarios.
 *
 * @param timestamp The ISO 8601 formatted timestamp string
 * @param error Output parameter that will contain an error message if parsing fails
 * @return A time_point representing the parsed timestamp
 */
std::chrono::system_clock::time_point parseISOTimestamp(const std::string& timestamp,
                                                        std::string& error);

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

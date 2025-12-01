#include "time_utils.hpp"
#include <cctype>
#include <iomanip>
#include <sstream>

namespace eppoclient {

std::chrono::system_clock::time_point parseISOTimestamp(
    const std::string& timestamp, std::chrono::system_clock::time_point fallbackTime) {
    std::tm tm = {};
    char dot = '\0';
    int milliseconds = 0;

    // Try parsing with optional fractional seconds
    std::istringstream ss(timestamp);

    // Read up to seconds first
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");

    if (ss.fail()) {
        return fallbackTime;
    }

    // Check if the next character is '.'
    if (ss.peek() == '.') {
        ss >> dot;
        // Parse digits after '.'
        std::string msDigits;
        // ISO8601 spec allows 1-6+ subsecond digits, but we want 3 (ms)
        for (int i = 0; i < 3; ++i) {
            char c = static_cast<char>(ss.peek());
            if (std::isdigit(c)) {
                msDigits.push_back(static_cast<char>(ss.get()));
            } else {
                break;
            }
        }
        // If less than 3 digits, pad with zeros
        while (msDigits.size() < 3)
            msDigits.push_back('0');
        milliseconds = std::stoi(msDigits);
        // The rest (additional sub-ms) is ignored
    }

    // If timestamp contains a timezone, ignore (assume UTC).
    // Use timegm for POSIX systems or _mkgmtime for Windows to interpret as UTC.

    std::time_t time_c;
#ifdef _WIN32
    time_c = _mkgmtime(&tm);
#else
    time_c = timegm(&tm);
#endif

    if (time_c == -1) {
        return fallbackTime;
    }
    auto tp = std::chrono::system_clock::from_time_t(time_c);
    tp += std::chrono::milliseconds(milliseconds);
    return tp;
}

std::string formatISOTimestamp(const std::chrono::system_clock::time_point& tp) {
    auto tt = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = {};
#ifdef _WIN32
    gmtime_s(&tm, &tt);
#else
    tm = *std::gmtime(&tt);
#endif

    // Add milliseconds
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()) % 1000;

    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    return ss.str();
}

}  // namespace eppoclient

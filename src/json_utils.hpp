#pragma once

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

namespace eppoclient {

// Internal namespace for implementation details.
// APIs in this namespace are NOT part of the public API contract and are NOT covered by semver.
// These may change or be removed at any time without notice.
namespace internal {

// Helper to get type name for error messages
template <typename T>
constexpr const char* getTypeName() {
    if constexpr (std::is_same_v<T, std::string>) {
        return "string";
    } else if constexpr (std::is_same_v<T, int> || std::is_same_v<T, int64_t>) {
        return "integer";
    } else if constexpr (std::is_same_v<T, double>) {
        return "number";
    } else if constexpr (std::is_same_v<T, bool>) {
        return "boolean";
    } else {
        return "unknown";
    }
}

// Helper to extract a typed value from a JSON iterator, with type validation
// Uses get_ref to avoid copies and prevent aborts with JSON_NOEXCEPTION
template <typename T>
std::optional<T> extractTypedValue(nlohmann::json::const_iterator it) {
    // Type validation using if constexpr (C++17)
    if constexpr (std::is_same_v<T, std::string>) {
        if (!it->is_string())
            return std::nullopt;
        return it->template get_ref<const std::string&>();
    } else if constexpr (std::is_same_v<T, int>) {
        if (!it->is_number_integer())
            return std::nullopt;
        return static_cast<int>(it->template get_ref<const nlohmann::json::number_integer_t&>());
    } else if constexpr (std::is_same_v<T, int64_t>) {
        if (!it->is_number_integer())
            return std::nullopt;
        return it->template get_ref<const nlohmann::json::number_integer_t&>();
    } else if constexpr (std::is_same_v<T, double>) {
        if (!it->is_number())
            return std::nullopt;
        // Handle both integer and float numbers
        if (it->is_number_float()) {
            return it->template get_ref<const nlohmann::json::number_float_t&>();
        } else {
            return static_cast<double>(
                it->template get_ref<const nlohmann::json::number_integer_t&>());
        }
    } else if constexpr (std::is_same_v<T, bool>) {
        if (!it->is_boolean())
            return std::nullopt;
        return it->template get_ref<const nlohmann::json::boolean_t&>();
    }

    return std::nullopt;
}

// Helper to get an optional field with type validation
template <typename T>
std::optional<T> getOptionalField(const nlohmann::json& j, std::string_view field) {
    auto it = j.find(field);
    if (it == j.end()) {
        return std::nullopt;
    }

    return extractTypedValue<T>(it);
}

// Helper to get a required field with error reporting (single lookup, uses string_view)
template <typename T>
std::optional<T> getRequiredField(const nlohmann::json& j, std::string_view field,
                                  std::string_view structName, std::string& error) {
    auto it = j.find(field);
    if (it == j.end()) {
        error = std::string(structName) + ": Missing required field: " + std::string(field);
        return std::nullopt;
    }

    auto value = extractTypedValue<T>(it);
    if (!value) {
        error = std::string(structName) + ": Field '" + std::string(field) + "' must be a " +
                getTypeName<T>();
        return std::nullopt;
    }

    return value;
}

// Macro to get required field and return nullopt on failure
#define TRY_GET_REQUIRED(var, type, json, field, struct_name, error)            \
    auto var = getRequiredField<type>((json), (field), (struct_name), (error)); \
    if (!var.has_value()) {                                                     \
        return std::nullopt;                                                    \
    }

// Macro to parse nested structures and return nullopt on failure
// Handles error message propagation from nested parsers
#define TRY_PARSE(var, parse_fn, json, error_prefix, error) \
    std::string var##_error;                                \
    auto var = (parse_fn)((json), var##_error);             \
    if (!var) {                                             \
        (error) = (error_prefix) + var##_error;             \
        return std::nullopt;                                \
    }

// Safe string-to-number conversion functions
// These functions safely parse numeric values from strings with comprehensive error checking:
// - Validates entire string is consumed (no trailing characters)
// - Detects overflow/underflow via errno
// - Returns std::nullopt on any parse failure

// Helper function to safely parse double from string
// Returns std::nullopt if parsing fails, including on overflow/underflow or trailing characters
inline std::optional<double> safeStrtod(const std::string& str) {
    char* endptr = nullptr;
    errno = 0;
    double result = std::strtod(str.c_str(), &endptr);

    // Check for conversion errors
    if (endptr == str.c_str()) {
        return std::nullopt;  // No conversion performed
    }

    // Check for trailing characters - entire string must be consumed
    if (*endptr != '\0') {
        return std::nullopt;  // Trailing garbage
    }

    // Check for overflow/underflow
    if (errno == ERANGE) {
        return std::nullopt;
    }

    return result;
}

// Helper function to safely parse int64_t from string
// Returns std::nullopt if parsing fails, including on overflow/underflow or trailing characters
inline std::optional<int64_t> safeStrtoll(const std::string& str, int base = 10) {
    char* endptr = nullptr;
    errno = 0;
    long long result = std::strtoll(str.c_str(), &endptr, base);

    // Check for conversion errors
    if (endptr == str.c_str()) {
        return std::nullopt;  // No conversion performed
    }

    // Check for trailing characters - entire string must be consumed
    if (*endptr != '\0') {
        return std::nullopt;  // Trailing garbage
    }

    // Check for overflow/underflow
    if (errno == ERANGE) {
        return std::nullopt;
    }

    // Additional check: ensure the value fits in int64_t
    // (On most platforms long long == int64_t, but not guaranteed)
    if (result > std::numeric_limits<int64_t>::max() ||
        result < std::numeric_limits<int64_t>::min()) {
        return std::nullopt;
    }

    return static_cast<int64_t>(result);
}

}  // namespace internal
}  // namespace eppoclient

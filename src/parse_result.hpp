#ifndef EPPOCLIENT_PARSE_RESULT_HPP
#define EPPOCLIENT_PARSE_RESULT_HPP

#include <optional>
#include <string>
#include <vector>

namespace eppoclient {

/**
 * Result type for parsing operations that may produce both a value and errors.
 *
 * This is used for parsing complex structures like ConfigResponse and BanditResponse,
 * where parsing may be partially successful (some items parsed, others failed).
 *
 * @tparam T The type of value being parsed
 */
template <typename T>
struct ParseResult {
    std::optional<T> value;
    std::vector<std::string> errors;

    // Helper methods
    bool hasValue() const { return value.has_value(); }
    bool hasErrors() const { return !errors.empty(); }

    // Convenience accessors
    T& operator*() { return *value; }
    const T& operator*() const { return *value; }
    T* operator->() { return &(*value); }
    const T* operator->() const { return &(*value); }
};

}  // namespace eppoclient

#endif  // EPPOCLIENT_PARSE_RESULT_HPP

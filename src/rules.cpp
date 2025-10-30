#include "rules.hpp"
#include "config_response.hpp"
#include <stdexcept>
#include <semver/semver.hpp>
#include <memory>
#include <regex>

namespace eppoclient {

// Rule matches if all conditions match
bool ruleMatches(const Rule& rule, const Attributes& subjectAttributes,
                 ApplicationLogger* logger) {
    for (const auto& condition : rule.conditions) {
        if (!conditionMatches(condition, subjectAttributes, logger)) {
            return false;
        }
    }
    return true;
}

// Condition matches based on operator and value comparison
bool conditionMatches(const Condition& condition, const Attributes& subjectAttributes,
                      ApplicationLogger* logger) {
    // Handle IS_NULL operator specially
    if (condition.operatorStr == "IS_NULL") {
        auto it = subjectAttributes.find(condition.attribute);
        bool isNull = (it == subjectAttributes.end() || std::holds_alternative<std::monostate>(it->second));

        // condition.value should be a boolean
        bool expectedNull = false;
        try {
            if (condition.value.is_boolean()) {
                expectedNull = condition.value.get<bool>();
            } else {
                return false;
            }
        } catch (...) {
            return false;
        }

        return isNull == expectedNull;
    }

    // For all other operators, the attribute must exist
    auto it = subjectAttributes.find(condition.attribute);
    if (it == subjectAttributes.end()) {
        return false;
    }

    const AttributeValue& subjectValue = it->second;

    // Handle different operators
    if (condition.operatorStr == "MATCHES") {
        std::string conditionValueStr;
        try {
            conditionValueStr = condition.value.get<std::string>();
        } catch (...) {
            return false;
        }
        return matches(subjectValue, conditionValueStr);

    } else if (condition.operatorStr == "NOT_MATCHES") {
        std::string conditionValueStr;
        try {
            conditionValueStr = condition.value.get<std::string>();
        } catch (...) {
            return false;
        }
        return !matches(subjectValue, conditionValueStr);

    } else if (condition.operatorStr == "ONE_OF") {
        std::vector<std::string> conditionArray = convertToStringArray(condition.value);
        return isOneOf(subjectValue, conditionArray);

    } else if (condition.operatorStr == "NOT_ONE_OF") {
        std::vector<std::string> conditionArray = convertToStringArray(condition.value);
        return !isOneOf(subjectValue, conditionArray);

    } else if (condition.operatorStr == "GTE" || condition.operatorStr == "GT" ||
               condition.operatorStr == "LTE" || condition.operatorStr == "LT") {

        // Try semver comparison first if subject is a string and condition has valid semver
        if (std::holds_alternative<std::string>(subjectValue) && condition.semVerValueValid) {
            const std::string& subjectValueStr = std::get<std::string>(subjectValue);

            try {
                semver::version<> subjectSemVer;
                auto result = semver::parse(subjectValueStr, subjectSemVer);
                if (result) {
                    return evaluateSemVerCondition(&subjectSemVer, condition.semVerValue.get(),
                                                  condition.operatorStr);
                }
            } catch (...) {
                // Failed to parse as semver, fall through to numeric comparison
            }
        }

        // Try numeric comparison
        try {
            double subjectValueNumeric = toDouble(subjectValue);
            if (condition.numericValueValid) {
                return evaluateNumericCondition(subjectValueNumeric, condition.numericValue,
                                               condition.operatorStr);
            }
        } catch (...) {
            // Not a numeric value either
        }

        // Neither numeric nor semver comparison worked
        return false;

    } else {
        // Unknown operator
        if (logger != nullptr) {
            logger->error("Unknown condition operator: " + condition.operatorStr);
        }
        return false;
    }
}

// Convert JSON array to string vector
std::vector<std::string> convertToStringArray(const nlohmann::json& conditionValue) {
    std::vector<std::string> result;

    if (conditionValue.is_array()) {
        for (const auto& item : conditionValue) {
            if (item.is_string()) {
                result.push_back(item.get<std::string>());
            } else {
                // Try to convert to string
                result.push_back(item.dump());
            }
        }
    }

    return result;
}

// Regex matching function
bool matches(const AttributeValue& subjectValue, const std::string& conditionValue) {
    std::string v;

    if (std::holds_alternative<std::string>(subjectValue)) {
        v = std::get<std::string>(subjectValue);
    } else if (std::holds_alternative<int64_t>(subjectValue)) {
        v = std::to_string(std::get<int64_t>(subjectValue));
    } else if (std::holds_alternative<bool>(subjectValue)) {
        v = std::get<bool>(subjectValue) ? "true" : "false";
    } else {
        return false;
    }

    try {
        std::regex pattern(conditionValue);
        return std::regex_search(v, pattern);
    } catch (const std::regex_error&) {
        return false;
    }
}

// Check if value is in array
bool isOneOf(const AttributeValue& attributeValue, const std::vector<std::string>& conditionValue) {
    for (const auto& value : conditionValue) {
        if (isOne(attributeValue, value)) {
            return true;
        }
    }
    return false;
}

// Check if value equals string (with type coercion)
bool isOne(const AttributeValue& attributeValue, const std::string& s) {
    if (std::holds_alternative<std::string>(attributeValue)) {
        return std::get<std::string>(attributeValue) == s;

    } else if (std::holds_alternative<double>(attributeValue)) {
        try {
            double value = std::stod(s);
            return std::get<double>(attributeValue) == value;
        } catch (...) {
            return false;
        }

    } else if (std::holds_alternative<int64_t>(attributeValue)) {
        try {
            int64_t value = std::stoll(s);
            return std::get<int64_t>(attributeValue) == value;
        } catch (...) {
            return false;
        }

    } else if (std::holds_alternative<bool>(attributeValue)) {
        // Parse boolean from string
        if (s == "true" || s == "True" || s == "TRUE" || s == "1") {
            return std::get<bool>(attributeValue) == true;
        } else if (s == "false" || s == "False" || s == "FALSE" || s == "0") {
            return std::get<bool>(attributeValue) == false;
        }
        return false;

    } else if (std::holds_alternative<std::monostate>(attributeValue)) {
        return s == "null" || s == "nil" || s.empty();

    } else {
        // Convert to string and compare
        return attributeValueToString(attributeValue) == s;
    }
}

// Semantic version comparison
bool evaluateSemVerCondition(const void* subjectValue, const void* conditionValue,
                             const std::string& operatorStr) {
    const semver::version<>* subject = static_cast<const semver::version<>*>(subjectValue);
    const semver::version<>* condition = static_cast<const semver::version<>*>(conditionValue);

    bool result = false;
    if (operatorStr == "GT") {
        result = *subject > *condition;
    } else if (operatorStr == "GTE") {
        result = *subject >= *condition;
    } else if (operatorStr == "LT") {
        result = *subject < *condition;
    } else if (operatorStr == "LTE") {
        result = *subject <= *condition;
    } else {
        throw std::runtime_error("Unknown operator for semver comparison: " + operatorStr);
    }

    return result;
}

// Numeric comparison
bool evaluateNumericCondition(double subjectValue, double conditionValue,
                              const std::string& operatorStr) {
    if (operatorStr == "GT") {
        return subjectValue > conditionValue;
    } else if (operatorStr == "GTE") {
        return subjectValue >= conditionValue;
    } else if (operatorStr == "LT") {
        return subjectValue < conditionValue;
    } else if (operatorStr == "LTE") {
        return subjectValue <= conditionValue;
    } else {
        throw std::runtime_error("Incorrect condition operator: " + operatorStr);
    }
}

// Convert AttributeValue to double
double toDouble(const AttributeValue& val) {
    if (std::holds_alternative<double>(val)) {
        return std::get<double>(val);

    } else if (std::holds_alternative<int64_t>(val)) {
        return static_cast<double>(std::get<int64_t>(val));

    } else if (std::holds_alternative<std::string>(val)) {
        try {
            return std::stod(std::get<std::string>(val));
        } catch (...) {
            throw std::runtime_error("Cannot convert string to double");
        }

    } else if (std::holds_alternative<bool>(val)) {
        return std::get<bool>(val) ? 1.0 : 0.0;

    } else {
        throw std::runtime_error("Value is neither a number nor a convertible string");
    }
}

// Convert nlohmann::json to double (overload for config_response.cpp)
double toDouble(const nlohmann::json& val) {
    if (val.is_number()) {
        return val.get<double>();
    } else if (val.is_string()) {
        try {
            return std::stod(val.get<std::string>());
        } catch (...) {
            throw std::runtime_error("Cannot convert string to double");
        }
    } else if (val.is_boolean()) {
        return val.get<bool>() ? 1.0 : 0.0;
    }
    throw std::runtime_error("Cannot convert value to double");
}

// Convert AttributeValue to string representation
std::string attributeValueToString(const AttributeValue& value) {
    if (std::holds_alternative<std::string>(value)) {
        return std::get<std::string>(value);
    } else if (std::holds_alternative<int64_t>(value)) {
        return std::to_string(std::get<int64_t>(value));
    } else if (std::holds_alternative<double>(value)) {
        return std::to_string(std::get<double>(value));
    } else if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value) ? "true" : "false";
    } else if (std::holds_alternative<std::monostate>(value)) {
        return "null";
    }
    return "";
}

} // namespace eppoclient

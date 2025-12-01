#include "rules.hpp"
#include <memory>
#include <regex>
#include <semver/semver.hpp>
#include "config_response.hpp"
#include "json_utils.hpp"

namespace eppoclient {
namespace internal {

// Rule matches if all conditions match
bool ruleMatches(const Rule& rule, const Attributes& subjectAttributes, ApplicationLogger* logger) {
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
    if (condition.op == Operator::IS_NULL) {
        auto it = subjectAttributes.find(condition.attribute);
        bool isNull =
            (it == subjectAttributes.end() || std::holds_alternative<std::monostate>(it->second));

        // condition.value should be a boolean
        if (!condition.value.is_boolean()) {
            return false;
        }
        bool expectedNull = condition.value.get<bool>();

        return isNull == expectedNull;
    }

    // For all other operators, the attribute must exist
    auto it = subjectAttributes.find(condition.attribute);
    if (it == subjectAttributes.end()) {
        return false;
    }

    const AttributeValue& subjectValue = it->second;

    // Handle different operators
    if (condition.op == Operator::MATCHES) {
        if (!condition.value.is_string()) {
            return false;
        }
        std::string conditionValueStr = condition.value.get<std::string>();
        return matches(subjectValue, conditionValueStr);

    } else if (condition.op == Operator::NOT_MATCHES) {
        if (!condition.value.is_string()) {
            return false;
        }
        std::string conditionValueStr = condition.value.get<std::string>();
        return !matches(subjectValue, conditionValueStr);

    } else if (condition.op == Operator::ONE_OF) {
        std::vector<std::string> conditionArray = convertToStringArray(condition.value);
        return isOneOf(subjectValue, conditionArray);

    } else if (condition.op == Operator::NOT_ONE_OF) {
        std::vector<std::string> conditionArray = convertToStringArray(condition.value);
        return !isOneOf(subjectValue, conditionArray);

    } else if (condition.op == Operator::GTE || condition.op == Operator::GT ||
               condition.op == Operator::LTE || condition.op == Operator::LT) {
        // Try semver comparison first if subject is a string and condition has valid semver
        if (std::holds_alternative<std::string>(subjectValue) && condition.semVerValueValid) {
            const std::string& subjectValueStr = std::get<std::string>(subjectValue);

            semver::version<> subjectSemVer;
            auto result = semver::parse(subjectValueStr, subjectSemVer);
            if (result) {
                return evaluateSemVerCondition(&subjectSemVer, condition.semVerValue.get(),
                                               condition.op);
            }
            // Failed to parse as semver, fall through to numeric comparison
        }

        // Try numeric comparison
        std::optional<double> subjectValueNumeric = tryToDouble(subjectValue);
        if (subjectValueNumeric.has_value() && condition.numericValueValid) {
            return evaluateNumericCondition(*subjectValueNumeric, condition.numericValue,
                                            condition.op);
        }

        // Neither numeric nor semver comparison worked
        return false;

    } else {
        // Unknown operator
        if (logger != nullptr) {
            logger->error("Unknown condition operator");
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

    // Note: With -fno-exceptions, regex construction will not throw
    // If the pattern is invalid, the behavior is implementation-defined
    std::regex pattern(conditionValue);
    return std::regex_search(v, pattern);
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
        // Try to parse string as double
        auto value = safeStrtod(s);
        if (!value.has_value()) {
            return false;  // Parse failed
        }
        return std::get<double>(attributeValue) == *value;

    } else if (std::holds_alternative<int64_t>(attributeValue)) {
        // Try to parse string as int64_t
        auto value = safeStrtoll(s);
        if (!value.has_value()) {
            return false;  // Parse failed
        }
        return std::get<int64_t>(attributeValue) == *value;

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
bool evaluateSemVerCondition(const void* subjectValue, const void* conditionValue, Operator op) {
    const semver::version<>* subject = static_cast<const semver::version<>*>(subjectValue);
    const semver::version<>* condition = static_cast<const semver::version<>*>(conditionValue);

    if (op == Operator::GT) {
        return *subject > *condition;
    } else if (op == Operator::GTE) {
        return *subject >= *condition;
    } else if (op == Operator::LT) {
        return *subject < *condition;
    } else if (op == Operator::LTE) {
        return *subject <= *condition;
    }

    // Unknown operator - should not reach here
    return false;
}

// Numeric comparison
bool evaluateNumericCondition(double subjectValue, double conditionValue, Operator op) {
    if (op == Operator::GT) {
        return subjectValue > conditionValue;
    } else if (op == Operator::GTE) {
        return subjectValue >= conditionValue;
    } else if (op == Operator::LT) {
        return subjectValue < conditionValue;
    } else if (op == Operator::LTE) {
        return subjectValue <= conditionValue;
    }

    // Unknown operator - should not reach here
    return false;
}

// Convert AttributeValue to double (returns std::nullopt if conversion fails)
std::optional<double> tryToDouble(const AttributeValue& val) {
    if (std::holds_alternative<double>(val)) {
        return std::get<double>(val);

    } else if (std::holds_alternative<int64_t>(val)) {
        return static_cast<double>(std::get<int64_t>(val));

    } else if (std::holds_alternative<std::string>(val)) {
        return safeStrtod(std::get<std::string>(val));

    } else if (std::holds_alternative<bool>(val)) {
        return std::get<bool>(val) ? 1.0 : 0.0;
    }

    return std::nullopt;
}

// Convert nlohmann::json to double (returns std::nullopt if conversion fails)
std::optional<double> tryToDouble(const nlohmann::json& val) {
    if (val.is_number()) {
        return val.get<double>();
    } else if (val.is_string()) {
        return safeStrtod(val.get<std::string>());
    } else if (val.is_boolean()) {
        return val.get<bool>() ? 1.0 : 0.0;
    }
    return std::nullopt;
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

}  // namespace internal
}  // namespace eppoclient

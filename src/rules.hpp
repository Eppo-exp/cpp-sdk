#ifndef RULES_HPP
#define RULES_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <nlohmann/json.hpp>
#include "application_logger.hpp"

namespace eppoclient {

// Forward declarations from config_response.h
enum class Operator;
struct Condition;
struct Rule;

// Type aliases for attribute values
// AttributeValue can be string, int64, double, bool, or null
using AttributeValue = std::variant<
    std::monostate,  // Represents null/nil
    std::string,
    int64_t,
    double,
    bool
>;

using Attributes = std::unordered_map<std::string, AttributeValue>;

// Rule matching functions
// Check if a rule matches the given subject attributes
bool ruleMatches(const Rule& rule, const Attributes& subjectAttributes,
                 ApplicationLogger* logger = nullptr);

// Check if a condition matches the given subject attributes
bool conditionMatches(const Condition& condition, const Attributes& subjectAttributes,
                      ApplicationLogger* logger = nullptr);

// Helper functions for condition evaluation
std::vector<std::string> convertToStringArray(const nlohmann::json& conditionValue);

bool matches(const AttributeValue& subjectValue, const std::string& conditionValue);

bool isOneOf(const AttributeValue& attributeValue, const std::vector<std::string>& conditionValue);

bool isOne(const AttributeValue& attributeValue, const std::string& s);

// Semantic version comparison
// Returns true/false for comparison, or throws exception on error
bool evaluateSemVerCondition(const void* subjectValue, const void* conditionValue,
                             Operator op);

// Numeric comparison
// Returns true/false for comparison, or throws exception on error
bool evaluateNumericCondition(double subjectValue, double conditionValue,
                              Operator op);

// Type conversion utilities
// Convert AttributeValue to double, throws exception on failure
double toDouble(const AttributeValue& val);

// Convert nlohmann::json to double (for use in config_response.cpp)
double toDouble(const nlohmann::json& val);

// Helper to convert AttributeValue to string representation
std::string attributeValueToString(const AttributeValue& value);

} // namespace eppoclient

#endif // RULES_H

#include "config_response.hpp"
#include "rules.hpp"
#include "time_utils.hpp"
#include <semver/semver.hpp>
#include <sstream>
#include <iomanip>
#include <cstdlib>

namespace eppoclient {

// VariationType JSON conversion
void to_json(nlohmann::json& j, const VariationType& vt) {
    switch (vt) {
        case VariationType::STRING:
            j = "STRING";
            break;
        case VariationType::INTEGER:
            j = "INTEGER";
            break;
        case VariationType::NUMERIC:
            j = "NUMERIC";
            break;
        case VariationType::BOOLEAN:
            j = "BOOLEAN";
            break;
        case VariationType::JSON:
            j = "JSON";
            break;
    }
}

void from_json(const nlohmann::json& j, VariationType& vt) {
    if (!j.is_string()) {
        vt = VariationType::STRING; // Default
        return;
    }
    std::string typeStr = j.get<std::string>();
    if (typeStr == "STRING") {
        vt = VariationType::STRING;
    } else if (typeStr == "INTEGER") {
        vt = VariationType::INTEGER;
    } else if (typeStr == "NUMERIC") {
        vt = VariationType::NUMERIC;
    } else if (typeStr == "BOOLEAN") {
        vt = VariationType::BOOLEAN;
    } else if (typeStr == "JSON") {
        vt = VariationType::JSON;
    } else {
        // Unknown type, default to STRING
        vt = VariationType::STRING;
    }
}

// Helper function to convert VariationType to string
std::string variationTypeToString(VariationType type) {
    switch (type) {
        case VariationType::STRING: return "STRING";
        case VariationType::INTEGER: return "INTEGER";
        case VariationType::NUMERIC: return "NUMERIC";
        case VariationType::BOOLEAN: return "BOOLEAN";
        case VariationType::JSON: return "JSON";
        default: return "UNKNOWN";
    }
}

// Helper function to detect the variation type
std::string detectVariationType(const std::variant<std::string, int64_t, double, bool, nlohmann::json>& variation) {
    if (std::holds_alternative<std::string>(variation)) return "STRING";
    if (std::holds_alternative<int64_t>(variation)) return "INTEGER";
    if (std::holds_alternative<double>(variation)) return "NUMERIC";
    if (std::holds_alternative<bool>(variation)) return "BOOLEAN";
    if (std::holds_alternative<nlohmann::json>(variation)) return "JSON";
    return "UNKNOWN";
}

// Operator JSON conversion
void to_json(nlohmann::json& j, const Operator& op) {
    switch (op) {
        case Operator::IS_NULL:
            j = "IS_NULL";
            break;
        case Operator::MATCHES:
            j = "MATCHES";
            break;
        case Operator::NOT_MATCHES:
            j = "NOT_MATCHES";
            break;
        case Operator::ONE_OF:
            j = "ONE_OF";
            break;
        case Operator::NOT_ONE_OF:
            j = "NOT_ONE_OF";
            break;
        case Operator::GTE:
            j = "GTE";
            break;
        case Operator::GT:
            j = "GT";
            break;
        case Operator::LTE:
            j = "LTE";
            break;
        case Operator::LT:
            j = "LT";
            break;
    }
}

void from_json(const nlohmann::json& j, Operator& op) {
    if (!j.is_string()) {
        op = Operator::ONE_OF; // Default
        return;
    }
    std::string opStr = j.get<std::string>();
    if (opStr == "IS_NULL") {
        op = Operator::IS_NULL;
    } else if (opStr == "MATCHES") {
        op = Operator::MATCHES;
    } else if (opStr == "NOT_MATCHES") {
        op = Operator::NOT_MATCHES;
    } else if (opStr == "ONE_OF") {
        op = Operator::ONE_OF;
    } else if (opStr == "NOT_ONE_OF") {
        op = Operator::NOT_ONE_OF;
    } else if (opStr == "GTE") {
        op = Operator::GTE;
    } else if (opStr == "GT") {
        op = Operator::GT;
    } else if (opStr == "LTE") {
        op = Operator::LTE;
    } else if (opStr == "LT") {
        op = Operator::LT;
    } else {
        // Unknown operator, default to ONE_OF
        op = Operator::ONE_OF;
    }
}

// Parse variation value based on type
// Returns std::nullopt if parsing fails
std::optional<std::variant<std::string, int64_t, double, bool, nlohmann::json>>
parseVariationValue(const nlohmann::json& value, VariationType type) {
    switch (type) {
        case VariationType::STRING:
            if (value.is_string()) {
                return value.get<std::string>();
            }
            return std::nullopt;

        case VariationType::INTEGER:
            if (value.is_number_integer()) {
                return value.get<int64_t>();
            } else if (value.is_number()) {
                // Check if the number is a float with a fractional part
                double d = value.get<double>();
                int64_t i = static_cast<int64_t>(d);
                // If the float has a fractional part, it's not a valid integer
                if (d != static_cast<double>(i)) {
                    return std::nullopt;
                }
                return i;
            } else if (value.is_string()) {
                const std::string& str = value.get<std::string>();
                char* end = nullptr;
                long long result = std::strtoll(str.c_str(), &end, 10);
                if (end == str.c_str() || *end != '\0') {
                    return std::nullopt;
                }
                return static_cast<int64_t>(result);
            }
            return std::nullopt;

        case VariationType::NUMERIC:
            if (value.is_number()) {
                return value.get<double>();
            } else if (value.is_string()) {
                const std::string& str = value.get<std::string>();
                char* end = nullptr;
                double result = std::strtod(str.c_str(), &end);
                if (end == str.c_str() || *end != '\0') {
                    return std::nullopt;
                }
                return result;
            }
            return std::nullopt;

        case VariationType::BOOLEAN:
            if (value.is_boolean()) {
                return value.get<bool>();
            } else if (value.is_string()) {
                std::string str = value.get<std::string>();
                if (str == "true" || str == "TRUE" || str == "True") {
                    return true;
                } else if (str == "false" || str == "FALSE" || str == "False") {
                    return false;
                }
                return std::nullopt;
            }
            return std::nullopt;

        case VariationType::JSON:
            // If the value is a string, parse it as JSON
            if (value.is_string()) {
                // With JSON_NOEXCEPTION, parse returns a discarded value on error
                auto parsed = nlohmann::json::parse(value.get<std::string>(), nullptr, false);
                if (parsed.is_discarded()) {
                    return std::nullopt;
                }
                return parsed;
            }
            // Otherwise, return the value as-is (it's already a JSON object)
            return value;
    }
    return std::nullopt;
}

// ShardRange JSON conversion
void to_json(nlohmann::json& j, const ShardRange& sr) {
    j = nlohmann::json{{"start", sr.start}, {"end", sr.end}};
}

void from_json(const nlohmann::json& j, ShardRange& sr) {
    j.at("start").get_to(sr.start);
    j.at("end").get_to(sr.end);
}

// Shard JSON conversion
void to_json(nlohmann::json& j, const Shard& s) {
    j = nlohmann::json{{"salt", s.salt}, {"ranges", s.ranges}};
}

void from_json(const nlohmann::json& j, Shard& s) {
    j.at("salt").get_to(s.salt);
    j.at("ranges").get_to(s.ranges);
}

// Split JSON conversion
void to_json(nlohmann::json& j, const Split& s) {
    j = nlohmann::json{
        {"shards", s.shards},
        {"variationKey", s.variationKey},
        {"extraLogging", s.extraLogging}
    };
}

void from_json(const nlohmann::json& j, Split& s) {
    j.at("shards").get_to(s.shards);
    j.at("variationKey").get_to(s.variationKey);
    if (j.contains("extraLogging")) {
        s.extraLogging = j.at("extraLogging");
    }
}

// Condition implementation
Condition::Condition()
    : numericValue(0.0),
      numericValueValid(false),
      semVerValue(nullptr),
      semVerValueValid(false) {}

void Condition::precompute() {
    // Try to parse as numeric value for performance
    numericValueValid = false;
    auto numericResult = tryToDouble(value);
    if (numericResult.has_value()) {
        numericValue = *numericResult;
        numericValueValid = true;
    }

    // Try to parse as semantic version if operator indicates version comparison
    semVerValueValid = false;
    if (op == Operator::GTE || op == Operator::GT ||
        op == Operator::LTE || op == Operator::LT) {

        // Try to parse the condition value as a semantic version
        if (value.is_string()) {
            auto version = std::make_shared<semver::version<>>();
            auto result = semver::parse(value.get<std::string>(), *version);
            if (result) {
                semVerValue = version;
                semVerValueValid = true;
            }
        }
    }
}

void to_json(nlohmann::json& j, const Condition& c) {
    j = nlohmann::json{
        {"operator", c.op},
        {"attribute", c.attribute},
        {"value", c.value}
    };
}

void from_json(const nlohmann::json& j, Condition& c) {
    j.at("operator").get_to(c.op);
    j.at("attribute").get_to(c.attribute);
    j.at("value").get_to(c.value);
}

// Rule JSON conversion
void to_json(nlohmann::json& j, const Rule& r) {
    j = nlohmann::json{{"conditions", r.conditions}};
}

void from_json(const nlohmann::json& j, Rule& r) {
    j.at("conditions").get_to(r.conditions);
}

// Allocation JSON conversion
void to_json(nlohmann::json& j, const Allocation& a) {
    j = nlohmann::json{
        {"key", a.key},
        {"rules", a.rules},
        {"splits", a.splits}
    };

    if (a.startAt.has_value()) {
        auto time = std::chrono::system_clock::to_time_t(a.startAt.value());
        j["startAt"] = time;
    }

    if (a.endAt.has_value()) {
        auto time = std::chrono::system_clock::to_time_t(a.endAt.value());
        j["endAt"] = time;
    }

    if (a.doLog.has_value()) {
        j["doLog"] = a.doLog.value();
    }
}

void from_json(const nlohmann::json& j, Allocation& a) {
    j.at("key").get_to(a.key);

    if (j.contains("rules")) {
        j.at("rules").get_to(a.rules);
    } else {
        a.rules.clear();
    }

    j.at("splits").get_to(a.splits);

    if (j.contains("startAt")) {
        auto timeStr = j.at("startAt").get<std::string>();
        a.startAt = parseISOTimestamp(timeStr);
    }

    if (j.contains("endAt")) {
        auto timeStr = j.at("endAt").get<std::string>();
        a.endAt = parseISOTimestamp(timeStr);
    }

    if (j.contains("doLog")) {
        a.doLog = j.at("doLog").get<bool>();
    }
}

// JsonVariationValue JSON conversion
void to_json(nlohmann::json& j, const JsonVariationValue& jvv) {
    j = jvv.value;
}

void from_json(const nlohmann::json& j, JsonVariationValue& jvv) {
    jvv.value = j;
}

// Variation JSON conversion
void to_json(nlohmann::json& j, const Variation& v) {
    j = nlohmann::json{
        {"key", v.key},
        {"value", v.value}
    };
}

void from_json(const nlohmann::json& j, Variation& v) {
    j.at("key").get_to(v.key);
    j.at("value").get_to(v.value);
}

// FlagConfiguration implementation
FlagConfiguration::FlagConfiguration()
    : enabled(false),
      variationType(),
      totalShards() {}

void FlagConfiguration::precompute() {
    // Parse and cache all variations for performance
    parsedVariations.clear();

    for (const auto& [varKey, variation] : variations) {
        auto parsed = parseVariationValue(variation.value, variationType);
        if (parsed.has_value()) {
            parsedVariations[varKey] = *parsed;
        }
        // Skip invalid variations
    }

    // Precompute conditions in all allocations
    for (auto& allocation : allocations) {
        for (auto& rule : allocation.rules) {
            for (auto& condition : rule.conditions) {
                condition.precompute();
            }
        }
    }
}

void to_json(nlohmann::json& j, const FlagConfiguration& fc) {
    j = nlohmann::json{
        {"key", fc.key},
        {"enabled", fc.enabled},
        {"variationType", fc.variationType},
        {"variations", fc.variations},
        {"allocations", fc.allocations},
        {"totalShards", fc.totalShards}
    };
}

void from_json(const nlohmann::json& j, FlagConfiguration& fc) {
    j.at("key").get_to(fc.key);
    j.at("enabled").get_to(fc.enabled);
    j.at("variationType").get_to(fc.variationType);
    j.at("variations").get_to(fc.variations);
    j.at("allocations").get_to(fc.allocations);
    j.at("totalShards").get_to(fc.totalShards);
}

// ConfigResponse implementation
void ConfigResponse::precompute() {
    // Precompute all flag configurations
    for (auto& [key, flagConfig] : flags) {
        flagConfig.precompute();
    }

    // Bandit variations don't require precomputation as they're simple data structures
}

void to_json(nlohmann::json& j, const ConfigResponse& cr) {
    j = nlohmann::json{
        {"flags", cr.flags},
        {"bandits", cr.bandits}
    };
}

void from_json(const nlohmann::json& j, ConfigResponse& cr) {
    if (j.contains("flags")) {
        j.at("flags").get_to(cr.flags);
    }

    if (j.contains("bandits")) {
        j.at("bandits").get_to(cr.bandits);
    }
}

} // namespace eppoclient

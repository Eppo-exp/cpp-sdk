#include "config_response.hpp"
#include <semver/semver.hpp>
#include "json_utils.hpp"
#include "rules.hpp"
#include "time_utils.hpp"

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

// Helper function to convert VariationType to string
std::string variationTypeToString(VariationType type) {
    switch (type) {
        case VariationType::STRING:
            return "STRING";
        case VariationType::INTEGER:
            return "INTEGER";
        case VariationType::NUMERIC:
            return "NUMERIC";
        case VariationType::BOOLEAN:
            return "BOOLEAN";
        case VariationType::JSON:
            return "JSON";
        default:
            return "UNKNOWN";
    }
}

// Helper function to detect the variation type
std::string detectVariationType(
    const std::variant<std::string, int64_t, double, bool, nlohmann::json>& variation) {
    if (std::holds_alternative<std::string>(variation))
        return "STRING";
    if (std::holds_alternative<int64_t>(variation))
        return "INTEGER";
    if (std::holds_alternative<double>(variation))
        return "NUMERIC";
    if (std::holds_alternative<bool>(variation))
        return "BOOLEAN";
    if (std::holds_alternative<nlohmann::json>(variation))
        return "JSON";
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

// Parse variation value based on type
// Returns std::nullopt if parsing fails
std::optional<std::variant<std::string, int64_t, double, bool, nlohmann::json>> parseVariationValue(
    const nlohmann::json& value, VariationType type) {
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
                return internal::safeStrtoll(value.get<std::string>());
            }
            return std::nullopt;

        case VariationType::NUMERIC:
            if (value.is_number()) {
                return value.get<double>();
            } else if (value.is_string()) {
                return internal::safeStrtod(value.get<std::string>());
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

// Internal namespace for implementation details not covered by semver
namespace internal {

// Custom parsing functions that handle errors gracefully

// Parse Operator enum with validation
std::optional<Operator> parseOperator(const nlohmann::json& j, std::string& error) {
    if (!j.is_string()) {
        error = "Operator must be a string";
        return std::nullopt;
    }

    const std::string& opStr = j.get_ref<const std::string&>();

    if (opStr == "IS_NULL")
        return Operator::IS_NULL;
    else if (opStr == "MATCHES")
        return Operator::MATCHES;
    else if (opStr == "NOT_MATCHES")
        return Operator::NOT_MATCHES;
    else if (opStr == "ONE_OF")
        return Operator::ONE_OF;
    else if (opStr == "NOT_ONE_OF")
        return Operator::NOT_ONE_OF;
    else if (opStr == "GTE")
        return Operator::GTE;
    else if (opStr == "GT")
        return Operator::GT;
    else if (opStr == "LTE")
        return Operator::LTE;
    else if (opStr == "LT")
        return Operator::LT;
    else {
        error = "Unknown operator: " + opStr;
        return std::nullopt;
    }
}

// Parse VariationType enum with validation
std::optional<VariationType> parseVariationType(const nlohmann::json& j, std::string& error) {
    if (!j.is_string()) {
        error = "VariationType must be a string";
        return std::nullopt;
    }

    const std::string& typeStr = j.get_ref<const std::string&>();

    if (typeStr == "STRING")
        return VariationType::STRING;
    else if (typeStr == "INTEGER")
        return VariationType::INTEGER;
    else if (typeStr == "NUMERIC")
        return VariationType::NUMERIC;
    else if (typeStr == "BOOLEAN")
        return VariationType::BOOLEAN;
    else if (typeStr == "JSON")
        return VariationType::JSON;
    else {
        error = "Unknown variationType: " + typeStr;
        return std::nullopt;
    }
}

std::optional<ShardRange> parseShardRange(const nlohmann::json& j, std::string& error) {
    TRY_GET_REQUIRED(start, int, j, "start", "ShardRange", error);
    TRY_GET_REQUIRED(end, int, j, "end", "ShardRange", error);

    ShardRange sr;
    sr.start = *start;
    sr.end = *end;
    return sr;
}

std::optional<Shard> parseShard(const nlohmann::json& j, std::string& error) {
    TRY_GET_REQUIRED(salt, std::string, j, "salt", "Shard", error);

    auto rangesIt = j.find("ranges");
    if (rangesIt == j.end()) {
        error = "Shard: Missing required field: ranges";
        return std::nullopt;
    }
    if (!rangesIt->is_array()) {
        error = "Shard: Field 'ranges' must be an array";
        return std::nullopt;
    }

    Shard s;
    s.salt = *salt;

    for (const auto& rangeJson : *rangesIt) {
        TRY_PARSE(range, parseShardRange, rangeJson, "Shard: Invalid range: ", error);
        s.ranges.push_back(*range);
    }

    return s;
}

std::optional<Split> parseSplit(const nlohmann::json& j, std::string& error) {
    auto shardsIt = j.find("shards");
    if (shardsIt == j.end()) {
        error = "Split: Missing required field: shards";
        return std::nullopt;
    }
    if (!shardsIt->is_array()) {
        error = "Split: Field 'shards' must be an array";
        return std::nullopt;
    }

    TRY_GET_REQUIRED(variationKey, std::string, j, "variationKey", "Split", error);

    Split s;
    s.variationKey = *variationKey;

    for (const auto& shardJson : *shardsIt) {
        TRY_PARSE(shard, parseShard, shardJson, "Split: Invalid shard: ", error);
        s.shards.push_back(*shard);
    }

    s.extraLogging = j.value("extraLogging", nlohmann::json::object());
    return s;
}

std::optional<Condition> parseCondition(const nlohmann::json& j, std::string& error) {
    auto opIt = j.find("operator");
    if (opIt == j.end()) {
        error = "Condition: Missing required field: operator";
        return std::nullopt;
    }

    TRY_PARSE(op, parseOperator, *opIt, "Condition: ", error);
    TRY_GET_REQUIRED(attribute, std::string, j, "attribute", "Condition", error);

    auto valueIt = j.find("value");
    if (valueIt == j.end()) {
        error = "Condition: Missing required field: value";
        return std::nullopt;
    }

    Condition c;
    c.op = *op;
    c.attribute = *attribute;
    c.value = *valueIt;  // value can be any JSON type
    return c;
}

std::optional<Rule> parseRule(const nlohmann::json& j, std::string& error) {
    auto conditionsIt = j.find("conditions");
    if (conditionsIt == j.end()) {
        error = "Rule: Missing required field: conditions";
        return std::nullopt;
    }
    if (!conditionsIt->is_array()) {
        error = "Rule: Field 'conditions' must be an array";
        return std::nullopt;
    }

    Rule r;
    for (const auto& condJson : *conditionsIt) {
        TRY_PARSE(condition, parseCondition, condJson, "Rule: Invalid condition: ", error);
        r.conditions.push_back(*condition);
    }

    return r;
}

std::optional<Allocation> parseAllocation(const nlohmann::json& j, std::string& error) {
    TRY_GET_REQUIRED(key, std::string, j, "key", "Allocation", error);

    auto splitsIt = j.find("splits");
    if (splitsIt == j.end()) {
        error = "Allocation: Missing required field: splits";
        return std::nullopt;
    }
    if (!splitsIt->is_array()) {
        error = "Allocation: Field 'splits' must be an array";
        return std::nullopt;
    }

    Allocation a;
    a.key = *key;

    // Parse optional rules
    auto rulesIt = j.find("rules");
    if (rulesIt != j.end()) {
        if (!rulesIt->is_array()) {
            error = "Allocation: Field 'rules' must be an array";
            return std::nullopt;
        }
        for (const auto& ruleJson : *rulesIt) {
            TRY_PARSE(rule, parseRule, ruleJson, "Allocation: Invalid rule: ", error);
            a.rules.push_back(*rule);
        }
    }

    // Parse required splits
    for (const auto& splitJson : *splitsIt) {
        TRY_PARSE(split, parseSplit, splitJson, "Allocation: Invalid split: ", error);
        a.splits.push_back(*split);
    }

    // Parse optional timestamp fields using getOptionalField
    auto startAt = getOptionalField<std::string>(j, "startAt");
    if (startAt) {
        a.startAt = parseISOTimestamp(*startAt, error);
    }

    auto endAt = getOptionalField<std::string>(j, "endAt");
    if (endAt) {
        a.endAt = parseISOTimestamp(*endAt, error);
    }

    auto doLog = getOptionalField<bool>(j, "doLog");
    if (doLog) {
        a.doLog = *doLog;
    }

    return a;
}

std::optional<Variation> parseVariation(const nlohmann::json& j, std::string& error) {
    TRY_GET_REQUIRED(key, std::string, j, "key", "Variation", error);

    auto valueIt = j.find("value");
    if (valueIt == j.end()) {
        error = "Variation: Missing required field: value";
        return std::nullopt;
    }

    Variation v;
    v.key = *key;
    v.value = *valueIt;  // value can be any JSON type
    return v;
}

std::optional<FlagConfiguration> parseFlagConfiguration(const nlohmann::json& j,
                                                        std::string& error) {
    TRY_GET_REQUIRED(key, std::string, j, "key", "FlagConfiguration", error);
    TRY_GET_REQUIRED(enabled, bool, j, "enabled", "FlagConfiguration", error);

    auto varTypeIt = j.find("variationType");
    if (varTypeIt == j.end()) {
        error = "FlagConfiguration: Missing required field: variationType";
        return std::nullopt;
    }

    TRY_PARSE(variationType, parseVariationType, *varTypeIt, "FlagConfiguration: ", error);

    auto variationsIt = j.find("variations");
    if (variationsIt == j.end()) {
        error = "FlagConfiguration: Missing required field: variations";
        return std::nullopt;
    }
    if (!variationsIt->is_object()) {
        error = "FlagConfiguration: Field 'variations' must be an object";
        return std::nullopt;
    }

    auto allocationsIt = j.find("allocations");
    if (allocationsIt == j.end()) {
        error = "FlagConfiguration: Missing required field: allocations";
        return std::nullopt;
    }
    if (!allocationsIt->is_array()) {
        error = "FlagConfiguration: Field 'allocations' must be an array";
        return std::nullopt;
    }

    // totalShards is optional, defaults to 10000
    int totalShards = getOptionalField<int>(j, "totalShards").value_or(10000);

    if (totalShards <= 0) {
        error = "FlagConfiguration: totalShards must be > 0, got " + std::to_string(totalShards);
        return std::nullopt;
    }

    FlagConfiguration fc;
    fc.key = *key;
    fc.enabled = *enabled;
    fc.variationType = *variationType;
    fc.totalShards = totalShards;

    // Parse variations
    for (auto& [varKey, varJson] : variationsIt->items()) {
        TRY_PARSE(variation, parseVariation, varJson,
                  "FlagConfiguration: Invalid variation '" + varKey + "': ", error);
        fc.variations[varKey] = *variation;
    }

    // Parse allocations
    for (const auto& allocJson : *allocationsIt) {
        TRY_PARSE(allocation, parseAllocation, allocJson,
                  "FlagConfiguration: Invalid allocation: ", error);
        fc.allocations.push_back(*allocation);
    }

    return fc;
}

}  // namespace internal

// ShardRange JSON conversion
void to_json(nlohmann::json& j, const ShardRange& sr) {
    j = nlohmann::json{{"start", sr.start}, {"end", sr.end}};
}

// Shard JSON conversion
void to_json(nlohmann::json& j, const Shard& s) {
    j = nlohmann::json{{"salt", s.salt}, {"ranges", s.ranges}};
}

// Split JSON conversion
void to_json(nlohmann::json& j, const Split& s) {
    j = nlohmann::json{
        {"shards", s.shards}, {"variationKey", s.variationKey}, {"extraLogging", s.extraLogging}};
}

// Condition implementation
Condition::Condition()
    : numericValue(0.0),
      numericValueValid(false),
      semVerValue(nullptr),
      semVerValueValid(false),
      regexValue(nullptr),
      regexValueValid(false) {}

void Condition::precompute() {
    // Try to parse as numeric value for performance
    numericValueValid = false;
    auto numericResult = internal::tryToDouble(value);
    if (numericResult.has_value()) {
        numericValue = *numericResult;
        numericValueValid = true;
    }

    // Try to parse as semantic version if operator indicates version comparison
    semVerValueValid = false;
    if (op == Operator::GTE || op == Operator::GT || op == Operator::LTE || op == Operator::LT) {
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

    // Precompile regex pattern for MATCHES/NOT_MATCHES operators using RE2
    // RE2 doesn't use exceptions - invalid patterns are detected via ok() method
    regexValueValid = false;
    if (op == Operator::MATCHES || op == Operator::NOT_MATCHES) {
        if (value.is_string()) {
            // Compile the RE2 pattern during precomputation
            // RE2 is exception-safe and will return ok() = false for invalid patterns
            auto pattern = std::make_shared<re2::RE2>(value.get<std::string>());
            if (pattern->ok()) {
                regexValue = pattern;
                regexValueValid = true;
            }
            // If !pattern->ok(), regexValueValid remains false
            // The condition will fail to match during evaluation, which is the correct behavior
        }
    }
}

void to_json(nlohmann::json& j, const Condition& c) {
    j = nlohmann::json{{"operator", c.op}, {"attribute", c.attribute}, {"value", c.value}};
}

// Rule JSON conversion
void to_json(nlohmann::json& j, const Rule& r) {
    j = nlohmann::json{{"conditions", r.conditions}};
}

// Allocation JSON conversion
void to_json(nlohmann::json& j, const Allocation& a) {
    j = nlohmann::json{{"key", a.key}, {"rules", a.rules}, {"splits", a.splits}};

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

// JsonVariationValue JSON conversion
void to_json(nlohmann::json& j, const JsonVariationValue& jvv) {
    j = jvv.value;
}

void from_json(const nlohmann::json& j, JsonVariationValue& jvv) {
    jvv.value = j;
}

// Variation JSON conversion
void to_json(nlohmann::json& j, const Variation& v) {
    j = nlohmann::json{{"key", v.key}, {"value", v.value}};
}

// FlagConfiguration implementation
FlagConfiguration::FlagConfiguration()
    : enabled(false), variationType(VariationType::STRING), totalShards(10000) {}

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
    j = nlohmann::json{{"key", fc.key},
                       {"enabled", fc.enabled},
                       {"variationType", fc.variationType},
                       {"variations", fc.variations},
                       {"allocations", fc.allocations},
                       {"totalShards", fc.totalShards}};
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
    j = nlohmann::json{{"flags", cr.flags}, {"bandits", cr.bandits}};
}

ParseResult<ConfigResponse> parseConfigResponse(const nlohmann::json& j) {
    ParseResult<ConfigResponse> result;
    ConfigResponse cr;

    // Parse flags - required field
    if (!j.contains("flags")) {
        result.errors.push_back("ConfigResponse: Missing required field: flags");
        return result;
    }

    if (!j["flags"].is_object()) {
        result.errors.push_back("ConfigResponse: 'flags' field must be an object");
        return result;
    }

    // Parse each flag independently, collect errors but continue parsing
    for (auto& [flagKey, flagJson] : j["flags"].items()) {
        std::string parseError;
        auto flag = internal::parseFlagConfiguration(flagJson, parseError);

        if (flag) {
            cr.flags[flagKey] = *flag;
        } else {
            // Report the error and continue parsing
            result.errors.push_back("Flag '" + flagKey + "': " + parseError);
        }
    }

    // Parse bandits - optional field
    if (j.contains("bandits")) {
        if (!j["bandits"].is_object()) {
            result.errors.push_back("ConfigResponse: 'bandits' field must be an object");
            return result;
        }

        for (auto& [banditKey, banditJsonArray] : j["bandits"].items()) {
            // Each bandit entry is an array of BanditVariation
            if (!banditJsonArray.is_array()) {
                result.errors.push_back("Bandit '" + banditKey +
                                        "': Expected an array of BanditVariation");
                continue;
            }

            std::vector<BanditVariation> variations;
            for (size_t i = 0; i < banditJsonArray.size(); ++i) {
                std::string parseError;
                auto banditVar = internal::parseBanditVariation(banditJsonArray[i], parseError);
                if (banditVar) {
                    variations.push_back(*banditVar);
                } else {
                    // Report the error and continue parsing
                    result.errors.push_back("Bandit '" + banditKey + "' variation[" +
                                            std::to_string(i) + "]: " + parseError);
                }
            }
            if (!variations.empty()) {
                cr.bandits[banditKey] = variations;
            }
        }
    }

    result.value = cr;
    return result;
}

ParseResult<ConfigResponse> parseConfigResponse(std::istream& is) {
    ParseResult<ConfigResponse> result;

    // Parse JSON from stream (with exceptions disabled)
    auto j = nlohmann::json::parse(is, nullptr, false);
    if (j.is_discarded()) {
        result.errors.push_back("JSON parse error: invalid JSON");
        return result;
    }

    // Delegate to the JSON-based parser
    return parseConfigResponse(j);
}

}  // namespace eppoclient

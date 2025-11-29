#ifndef CONFIG_RESPONSE_HPP
#define CONFIG_RESPONSE_HPP

#include <chrono>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include "bandit_model.hpp"

namespace eppoclient {

// Enum for variation types
enum class VariationType { STRING, INTEGER, NUMERIC, BOOLEAN, JSON };

// serialization for the nlohmann::json library
void to_json(nlohmann::json& j, const VariationType& vt);

// Helper functions for VariationType
std::string variationTypeToString(VariationType type);
std::string detectVariationType(
    const std::variant<std::string, int64_t, double, bool, nlohmann::json>& variation);

// Parse variation value based on type
// Returns std::nullopt if parsing fails
std::optional<std::variant<std::string, int64_t, double, bool, nlohmann::json>> parseVariationValue(
    const nlohmann::json& value, VariationType type);

// Shard range structure
struct ShardRange {
    int start;
    int end;
};

// serialization for the nlohmann::json library
void to_json(nlohmann::json& j, const ShardRange& sr);

// Operator enum
enum class Operator { IS_NULL, MATCHES, NOT_MATCHES, ONE_OF, NOT_ONE_OF, GTE, GT, LTE, LT };

// serialization for the nlohmann::json library
void to_json(nlohmann::json& j, const Operator& op);

// Shard structure
struct Shard {
    std::string salt;
    std::vector<ShardRange> ranges;
};

// serialization for the nlohmann::json library
void to_json(nlohmann::json& j, const Shard& s);

// Split structure
struct Split {
    std::vector<Shard> shards;
    std::string variationKey;
    nlohmann::json extraLogging;
};

// serialization for the nlohmann::json library
void to_json(nlohmann::json& j, const Split& s);

// Condition structure
struct Condition {
    Operator op;  // operator is a C++ keyword, using op
    std::string attribute;
    nlohmann::json value;

    // Cached values for performance (not serialized)
    double numericValue;
    bool numericValueValid;
    std::shared_ptr<void> semVerValue;
    bool semVerValueValid;

    Condition();
    void precompute();
};

// serialization for the nlohmann::json library
void to_json(nlohmann::json& j, const Condition& c);

// Rule structure - contains multiple conditions (AND logic)
struct Rule {
    std::vector<Condition> conditions;
};

// serialization for the nlohmann::json library
void to_json(nlohmann::json& j, const Rule& r);

// Allocation structure
struct Allocation {
    std::string key;
    std::vector<Rule> rules;
    std::optional<std::chrono::system_clock::time_point> startAt;
    std::optional<std::chrono::system_clock::time_point> endAt;
    std::vector<Split> splits;
    std::optional<bool> doLog;
};

// serialization for the nlohmann::json library
void to_json(nlohmann::json& j, const Allocation& a);

// JSON variation value structure
struct JsonVariationValue {
    nlohmann::json value;
};

// serialization/deserialization for the nlohmann::json library
void to_json(nlohmann::json& j, const JsonVariationValue& jvv);
void from_json(const nlohmann::json& j, JsonVariationValue& jvv);

// Variation structure
struct Variation {
    std::string key;
    nlohmann::json value;
};

// serialization for the nlohmann::json library
void to_json(nlohmann::json& j, const Variation& v);

// Flag configuration structure
struct FlagConfiguration {
    std::string key;
    bool enabled;
    VariationType variationType;
    std::unordered_map<std::string, Variation> variations;
    std::vector<Allocation> allocations;
    int totalShards;

    // Cached parsed variations (not serialized)
    std::unordered_map<std::string,
                       std::variant<std::string, int64_t, double, bool, nlohmann::json>>
        parsedVariations;

    FlagConfiguration();
    void precompute();
};

// serialization for the nlohmann::json library
void to_json(nlohmann::json& j, const FlagConfiguration& fc);

// Top-level config response structure
struct ConfigResponse {
    std::unordered_map<std::string, FlagConfiguration> flags;
    std::unordered_map<std::string, std::vector<BanditVariation>> bandits;

    void precompute();
};

// serialization/deserialization for the nlohmann::json library
void to_json(nlohmann::json& j, const ConfigResponse& cr);
void from_json(const nlohmann::json& j, ConfigResponse& cr);

// Internal namespace for implementation details not covered by semver
namespace internal {

// Custom parsing functions that handle errors gracefully.
// These are INTERNAL APIs and may change without notice.
std::optional<ShardRange> parseShardRange(const nlohmann::json& j, std::string& error);
std::optional<Shard> parseShard(const nlohmann::json& j, std::string& error);
std::optional<Split> parseSplit(const nlohmann::json& j, std::string& error);
std::optional<Condition> parseCondition(const nlohmann::json& j, std::string& error);
std::optional<Rule> parseRule(const nlohmann::json& j, std::string& error);
std::optional<Allocation> parseAllocation(const nlohmann::json& j, std::string& error);
std::optional<Variation> parseVariation(const nlohmann::json& j, std::string& error);
std::optional<FlagConfiguration> parseFlagConfiguration(const nlohmann::json& j,
                                                        std::string& error);
std::optional<Operator> parseOperator(const nlohmann::json& j, std::string& error);
std::optional<VariationType> parseVariationType(const nlohmann::json& j, std::string& error);

}  // namespace internal

}  // namespace eppoclient

#endif  // CONFIG_RESPONSE_H

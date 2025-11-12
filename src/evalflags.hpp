#ifndef EVALFLAGS_HPP
#define EVALFLAGS_HPP

#include <string>
#include <unordered_map>
#include <optional>
#include <variant>
#include <chrono>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include "application_logger.hpp"
#include "config_response.hpp"
#include "rules.hpp"

namespace eppoclient {

// SDK version constant
extern const std::string SDK_VERSION;

// Error types for flag evaluation
class FlagNotEnabledException : public std::runtime_error {
public:
    FlagNotEnabledException() : std::runtime_error("the experiment or flag is not enabled") {}
};

class SubjectAllocationException : public std::runtime_error {
public:
    SubjectAllocationException() : std::runtime_error("subject is not part of any allocation") {}
};

// AssignmentEvent structure for logging
struct AssignmentEvent {
    std::string experiment;
    std::string featureFlag;
    std::string allocation;
    std::string variation;
    std::string subject;
    Attributes subjectAttributes;
    std::string timestamp;
    std::unordered_map<std::string, std::string> metaData;
    std::unordered_map<std::string, std::string> extraLogging;
};

// Evaluation codes
enum class FlagEvaluationCode {
    MATCH,
    CONFIGURATION_MISSING,
    FLAG_UNRECOGNIZED_OR_DISABLED,
    DEFAULT_ALLOCATION_NULL,
    TYPE_MISMATCH,
    UNEXPECTED_CONFIGURATION_ERROR
};

enum class BanditEvaluationCode {
    MATCH,
    CONFIGURATION_MISSING,
    UNEXPECTED_CONFIGURATION_ERROR,
    NON_BANDIT_VARIATION,
    NO_ACTIONS_SUPPLIED_FOR_BANDIT
};

enum class AllocationEvaluationCode {
    UNEVALUATED,
    MATCH,
    BEFORE_START_TIME,
    AFTER_END_TIME,
    FAILING_RULE,
    TRAFFIC_EXPOSURE_MISS
};

// Forward declarations for evaluation details
struct ConditionEvaluationDetails {
    nlohmann::json condition;
    std::optional<std::variant<std::string, int64_t, double, bool>> attributeValue;
    bool matched;
};

struct RuleEvaluationDetails {
    bool matched;
    std::vector<ConditionEvaluationDetails> conditions;
};

struct ShardEvaluationDetails {
    bool matched;
    nlohmann::json shard;
    uint32_t shardValue;
};

struct SplitEvaluationDetails {
    std::string variationKey;
    bool matched;
    std::vector<ShardEvaluationDetails> shards;
};

struct AllocationEvaluationDetails {
    std::string key;
    size_t orderPosition;
    AllocationEvaluationCode allocationEvaluationCode;
    std::vector<RuleEvaluationDetails> evaluatedRules;
    std::vector<SplitEvaluationDetails> evaluatedSplits;
};

// Evaluation details structure
struct EvaluationDetails {
    std::string flagKey;
    std::string subjectKey;
    Attributes subjectAttributes;
    std::string timestamp;

    std::optional<std::string> configFetchedAt;
    std::optional<std::string> configPublishedAt;
    std::optional<std::string> environmentName;

    std::optional<BanditEvaluationCode> banditEvaluationCode;
    std::optional<FlagEvaluationCode> flagEvaluationCode;
    std::string flagEvaluationDescription;

    std::optional<std::string> variationKey;
    std::optional<std::variant<std::string, int64_t, double, bool, nlohmann::json>> variationValue;

    std::optional<std::string> banditKey;
    std::optional<std::string> banditAction;

    std::vector<AllocationEvaluationDetails> allocations;
};

// Result type for flag evaluation
struct EvalResult {
    std::variant<std::string, int64_t, double, bool, nlohmann::json> value;
    std::optional<AssignmentEvent> event;
};

// Result with details
struct EvalResultWithDetails {
    std::optional<std::variant<std::string, int64_t, double, bool, nlohmann::json>> value;
    std::optional<AssignmentEvent> event;
    EvaluationDetails details;
};

// Helper functions for sharding
int64_t getShard(const std::string& input, int64_t totalShards);
bool isShardInRange(int64_t shard, const ShardRange& range);

// Augment subject attributes with subject key
Attributes augmentWithSubjectKey(const Attributes& subjectAttributes, const std::string& subjectKey);

// FlagConfiguration member functions
// Verify that the flag has the expected variation type
void verifyType(const FlagConfiguration& flag, VariationType expectedType);

// Evaluate a flag for a given subject
EvalResult evalFlag(const FlagConfiguration& flag,
                   const std::string& subjectKey,
                   const Attributes& subjectAttributes,
                   ApplicationLogger* logger = nullptr);

// Evaluate a flag and return detailed evaluation information
EvalResultWithDetails evalFlagDetails(const FlagConfiguration& flag,
                                     const std::string& subjectKey,
                                     const Attributes& subjectAttributes,
                                     ApplicationLogger* logger = nullptr);

// Allocation member functions
// Find a matching split for the given subject
const Split* findMatchingSplit(const Allocation& allocation,
                               const std::string& subjectKey,
                               const Attributes& augmentedSubjectAttributes,
                               int64_t totalShards,
                               const std::chrono::system_clock::time_point& now,
                               ApplicationLogger* logger = nullptr);

// Split member functions
// Check if a split matches the given subject
bool splitMatches(const Split& split, const std::string& subjectKey, int64_t totalShards);

// Shard member functions
// Check if a shard matches the given subject
bool shardMatches(const Shard& shard, const std::string& subjectKey, int64_t totalShards);

} // namespace eppoclient

#endif // EVALFLAGS_H

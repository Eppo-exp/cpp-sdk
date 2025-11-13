#include "evalflags.hpp"
#include "version.hpp"
#include "time_utils.hpp"
#include "third_party/md5_wrapper.h"
#include <cstring>

namespace eppoclient {

const std::string SDK_VERSION = getVersion();

// Verify that the flag has the expected variation type
void verifyType(const FlagConfiguration& flag, VariationType expectedType) {
    if (flag.variationType != expectedType) {
        throw std::runtime_error(
            "unexpected variation type (expected: " +
            variationTypeToString(expectedType) +
            ", actual: " +
            variationTypeToString(flag.variationType) + ")"
        );
    }
}

// Evaluate a flag for a given subject
EvalResult evalFlag(const FlagConfiguration& flag,
                   const std::string& subjectKey,
                   const Attributes& subjectAttributes,
                   ApplicationLogger* logger) {
    // Check if flag is enabled
    if (!flag.enabled) {
        throw FlagNotEnabledException();
    }

    auto now = std::chrono::system_clock::now();
    Attributes augmentedSubjectAttributes = augmentWithSubjectKey(subjectAttributes, subjectKey);

    // Find matching allocation and split
    const Allocation* matchedAllocation = nullptr;
    const Split* matchedSplit = nullptr;

    for (const auto& allocation : flag.allocations) {
        const Split* split = findMatchingSplit(
            allocation,
            subjectKey,
            augmentedSubjectAttributes,
            flag.totalShards,
            now,
            logger
        );
        if (split != nullptr) {
            matchedAllocation = &allocation;
            matchedSplit = split;
            break;
        }
    }

    if (matchedAllocation == nullptr || matchedSplit == nullptr) {
        throw SubjectAllocationException();
    }

    // Find the variation value
    auto it = flag.parsedVariations.find(matchedSplit->variationKey);
    if (it == flag.parsedVariations.end()) {
        throw std::runtime_error("cannot find variation: " + matchedSplit->variationKey);
    }

    EvalResult result;
    result.value = it->second;

    // Create assignment event if logging is enabled
    bool shouldLog = true;
    if (matchedAllocation->doLog.has_value()) {
        shouldLog = matchedAllocation->doLog.value();
    }

    if (shouldLog) {
        AssignmentEvent event;
        event.featureFlag = flag.key;
        event.allocation = matchedAllocation->key;
        event.experiment = flag.key + "-" + matchedAllocation->key;
        event.variation = matchedSplit->variationKey;
        event.subject = subjectKey;
        event.subjectAttributes = subjectAttributes;
        event.timestamp = formatISOTimestamp(now);
        event.metaData = {
            {"sdkLanguage", "cpp"},
            {"sdkVersion", SDK_VERSION}
        };

        // Convert extraLogging JSON to map of strings
        if (matchedSplit->extraLogging.is_object()) {
            for (auto& [key, value] : matchedSplit->extraLogging.items()) {
                if (value.is_string()) {
                    event.extraLogging[key] = value.get<std::string>();
                } else {
                    event.extraLogging[key] = value.dump();
                }
            }
        }

        result.event = event;
    }

    return result;
}

// Helper function to evaluate allocation with details
AllocationEvaluationDetails evaluateAllocationWithDetails(
    const Allocation& allocation,
    const std::string& subjectKey,
    const Attributes& augmentedSubjectAttributes,
    int64_t totalShards,
    const std::chrono::system_clock::time_point& now,
    size_t orderPosition,
    ApplicationLogger* logger) {

    AllocationEvaluationDetails details;
    details.key = allocation.key;
    details.orderPosition = orderPosition;

    // Check time constraints
    if (allocation.startAt.has_value() && now < allocation.startAt.value()) {
        details.allocationEvaluationCode = AllocationEvaluationCode::BEFORE_START_TIME;
        return details;
    }
    if (allocation.endAt.has_value() && now > allocation.endAt.value()) {
        details.allocationEvaluationCode = AllocationEvaluationCode::AFTER_END_TIME;
        return details;
    }

    // Check if any rule matches
    bool matchesRule = false;
    if (!allocation.rules.empty()) {
        for (const auto& rule : allocation.rules) {
            if (ruleMatches(rule, augmentedSubjectAttributes, logger)) {
                matchesRule = true;
                break;
            }
        }
        if (!matchesRule) {
            details.allocationEvaluationCode = AllocationEvaluationCode::FAILING_RULE;
            return details;
        }
    }

    // Find matching split
    bool foundMatchingSplit = false;
    for (const auto& split : allocation.splits) {
        if (splitMatches(split, subjectKey, totalShards)) {
            foundMatchingSplit = true;
            break;
        }
    }

    if (!foundMatchingSplit) {
        details.allocationEvaluationCode = AllocationEvaluationCode::TRAFFIC_EXPOSURE_MISS;
        return details;
    }

    details.allocationEvaluationCode = AllocationEvaluationCode::MATCH;
    return details;
}

// Evaluate a flag and return detailed evaluation information
EvalResultWithDetails evalFlagDetails(const FlagConfiguration& flag,
                                     const std::string& subjectKey,
                                     const Attributes& subjectAttributes,
                                     ApplicationLogger* logger) {
    auto now = std::chrono::system_clock::now();
    std::string timestamp = formatISOTimestamp(now);

    EvalResultWithDetails result;
    result.details.flagKey = flag.key;
    result.details.subjectKey = subjectKey;
    result.details.subjectAttributes = subjectAttributes;
    result.details.timestamp = timestamp;

    // Check if flag is enabled
    if (!flag.enabled) {
        result.details.flagEvaluationCode = FlagEvaluationCode::FLAG_UNRECOGNIZED_OR_DISABLED;
        result.details.flagEvaluationDescription = "Flag is not enabled";
        return result;
    }

    Attributes augmentedSubjectAttributes = augmentWithSubjectKey(subjectAttributes, subjectKey);

    // Evaluate all allocations and track details
    const Allocation* matchedAllocation = nullptr;
    const Split* matchedSplit = nullptr;

    for (size_t i = 0; i < flag.allocations.size(); ++i) {
        const auto& allocation = flag.allocations[i];

        // Track allocation evaluation details
        AllocationEvaluationDetails allocDetails = evaluateAllocationWithDetails(
            allocation,
            subjectKey,
            augmentedSubjectAttributes,
            flag.totalShards,
            now,
            i,
            logger
        );
        result.details.allocations.push_back(allocDetails);

        // If this allocation matched and we don't have a match yet, use it
        if (allocDetails.allocationEvaluationCode == AllocationEvaluationCode::MATCH &&
            matchedAllocation == nullptr) {
            const Split* split = findMatchingSplit(
                allocation,
                subjectKey,
                augmentedSubjectAttributes,
                flag.totalShards,
                now,
                logger
            );
            if (split != nullptr) {
                matchedAllocation = &allocation;
                matchedSplit = split;
            }
        }
    }

    if (matchedAllocation == nullptr || matchedSplit == nullptr) {
        result.details.flagEvaluationCode = FlagEvaluationCode::DEFAULT_ALLOCATION_NULL;
        result.details.flagEvaluationDescription = "Subject is not part of any allocation";
        return result;
    }

    // Find the variation value
    auto it = flag.parsedVariations.find(matchedSplit->variationKey);
    if (it == flag.parsedVariations.end()) {
        result.details.flagEvaluationCode = FlagEvaluationCode::UNEXPECTED_CONFIGURATION_ERROR;
        result.details.flagEvaluationDescription = "Cannot find variation: " + matchedSplit->variationKey;
        return result;
    }

    result.value = it->second;
    result.details.variationKey = matchedSplit->variationKey;
    result.details.variationValue = it->second;
    result.details.flagEvaluationCode = FlagEvaluationCode::MATCH;
    result.details.flagEvaluationDescription = "Flag evaluation successful";

    // Create assignment event if logging is enabled
    bool shouldLog = true;
    if (matchedAllocation->doLog.has_value()) {
        shouldLog = matchedAllocation->doLog.value();
    }

    if (shouldLog) {
        AssignmentEvent event;
        event.featureFlag = flag.key;
        event.allocation = matchedAllocation->key;
        event.experiment = flag.key + "-" + matchedAllocation->key;
        event.variation = matchedSplit->variationKey;
        event.subject = subjectKey;
        event.subjectAttributes = subjectAttributes;
        event.timestamp = timestamp;
        event.metaData = {
            {"sdkLanguage", "cpp"},
            {"sdkVersion", SDK_VERSION}
        };

        // Convert extraLogging JSON to map of strings
        if (matchedSplit->extraLogging.is_object()) {
            for (auto& [key, value] : matchedSplit->extraLogging.items()) {
                if (value.is_string()) {
                    event.extraLogging[key] = value.get<std::string>();
                } else {
                    event.extraLogging[key] = value.dump();
                }
            }
        }

        result.event = event;
    }

    return result;
}

// Augment subject attributes by setting "id" attribute to subjectKey
// if "id" is not already present
Attributes augmentWithSubjectKey(const Attributes& subjectAttributes, const std::string& subjectKey) {
    auto it = subjectAttributes.find("id");
    if (it != subjectAttributes.end()) {
        // "id" already exists, return as-is
        return subjectAttributes;
    }

    // Create a copy and add "id"
    Attributes augmented = subjectAttributes;
    augmented["id"] = subjectKey;
    return augmented;
}

// Find a matching split for the given subject
const Split* findMatchingSplit(const Allocation& allocation,
                               const std::string& subjectKey,
                               const Attributes& augmentedSubjectAttributes,
                               int64_t totalShards,
                               const std::chrono::system_clock::time_point& now,
                               ApplicationLogger* logger) {
    // Check time constraints
    if (allocation.startAt.has_value() && now < allocation.startAt.value()) {
        return nullptr;
    }
    if (allocation.endAt.has_value() && now > allocation.endAt.value()) {
        return nullptr;
    }

    // Check if any rule matches
    bool matchesRule = false;
    for (const auto& rule : allocation.rules) {
        if (ruleMatches(rule, augmentedSubjectAttributes, logger)) {
            matchesRule = true;
            break;
        }
    }

    // If there are rules and none match, return nullptr
    if (!allocation.rules.empty() && !matchesRule) {
        return nullptr;
    }

    // Find matching split
    for (const auto& split : allocation.splits) {
        if (splitMatches(split, subjectKey, totalShards)) {
            return &split;
        }
    }

    return nullptr;
}

// Check if a split matches the given subject
bool splitMatches(const Split& split, const std::string& subjectKey, int64_t totalShards) {
    for (const auto& shard : split.shards) {
        if (!shardMatches(shard, subjectKey, totalShards)) {
            return false;
        }
    }
    return true;
}

// Check if a shard matches the given subject
bool shardMatches(const Shard& shard, const std::string& subjectKey, int64_t totalShards) {
    int64_t s = getShard(shard.salt + "-" + subjectKey, totalShards);
    for (const auto& range : shard.ranges) {
        if (isShardInRange(s, range)) {
            return true;
        }
    }
    return false;
}

// Get shard value using MD5 hash
int64_t getShard(const std::string& input, int64_t totalShards) {
    unsigned char hash[MD5_DIGEST_LENGTH];
    md5_hash(input.c_str(), input.length(), hash);

    // Use first 4 bytes of MD5 hash in big-endian format
    uint32_t intVal = (static_cast<uint32_t>(hash[0]) << 24) |
                      (static_cast<uint32_t>(hash[1]) << 16) |
                      (static_cast<uint32_t>(hash[2]) << 8) |
                      static_cast<uint32_t>(hash[3]);

    return static_cast<int64_t>(intVal) % totalShards;
}

// Check if shard is within the given range
bool isShardInRange(int64_t shard, const ShardRange& range) {
    return shard >= range.start && shard < range.end;
}

// Helper function to convert FlagEvaluationCode to string
std::string flagEvaluationCodeToString(FlagEvaluationCode code) {
    switch (code) {
        case FlagEvaluationCode::MATCH:
            return "MATCH";
        case FlagEvaluationCode::CONFIGURATION_MISSING:
            return "CONFIGURATION_MISSING";
        case FlagEvaluationCode::FLAG_UNRECOGNIZED_OR_DISABLED:
            return "FLAG_UNRECOGNIZED_OR_DISABLED";
        case FlagEvaluationCode::DEFAULT_ALLOCATION_NULL:
            return "DEFAULT_ALLOCATION_NULL";
        case FlagEvaluationCode::TYPE_MISMATCH:
            return "TYPE_MISMATCH";
        case FlagEvaluationCode::UNEXPECTED_CONFIGURATION_ERROR:
            return "UNEXPECTED_CONFIGURATION_ERROR";
        default:
            return "UNKNOWN";
    }
}

// Helper function to convert string to FlagEvaluationCode
// Returns std::nullopt if the code string is not recognized
std::optional<FlagEvaluationCode> stringToFlagEvaluationCode(const std::string& codeStr) {
    if (codeStr == "MATCH") return FlagEvaluationCode::MATCH;
    if (codeStr == "CONFIGURATION_MISSING") return FlagEvaluationCode::CONFIGURATION_MISSING;
    if (codeStr == "FLAG_UNRECOGNIZED_OR_DISABLED") return FlagEvaluationCode::FLAG_UNRECOGNIZED_OR_DISABLED;
    if (codeStr == "DEFAULT_ALLOCATION_NULL") return FlagEvaluationCode::DEFAULT_ALLOCATION_NULL;
    if (codeStr == "TYPE_MISMATCH") return FlagEvaluationCode::TYPE_MISMATCH;
    if (codeStr == "UNEXPECTED_CONFIGURATION_ERROR") return FlagEvaluationCode::UNEXPECTED_CONFIGURATION_ERROR;
    // Test cases use "ASSIGNMENT_ERROR" for js/node SDKs, but it should map to UNEXPECTED_CONFIGURATION_ERROR
    if (codeStr == "ASSIGNMENT_ERROR") return FlagEvaluationCode::UNEXPECTED_CONFIGURATION_ERROR;
    // Return nullopt for unknown codes
    return std::nullopt;
}

} // namespace eppoclient

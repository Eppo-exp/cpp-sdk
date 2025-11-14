#include "evalflags.hpp"
#include "src/version.hpp"
#include "third_party/md5_wrapper.h"
#include <iomanip>
#include <sstream>
#include <cstring>

namespace eppoclient {

const std::string SDK_VERSION = getVersion();

// Convert time_point to RFC3339 string
std::string toRFC3339(const std::chrono::system_clock::time_point& tp) {
    auto time_t_val = std::chrono::system_clock::to_time_t(tp);
    std::tm tm_utc;
    gmtime_r(&time_t_val, &tm_utc);

    std::ostringstream oss;
    oss << std::put_time(&tm_utc, "%Y-%m-%dT%H:%M:%S");

    // Add milliseconds
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        tp.time_since_epoch()) % 1000;
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';

    return oss.str();
}

// Verify that the flag has the expected variation type
void verifyType(const FlagConfiguration& flag, VariationType expectedType) {
    if (flag.variationType != expectedType) {
        throw std::runtime_error(
            "unexpected variation type (expected: " +
            std::to_string(static_cast<int>(expectedType)) +
            ", actual: " +
            std::to_string(static_cast<int>(flag.variationType)) + ")"
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
        event.timestamp = toRFC3339(now);
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

} // namespace eppoclient

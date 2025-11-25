#ifndef LRU_ASSIGNMENT_LOGGER_HPP
#define LRU_ASSIGNMENT_LOGGER_HPP

#include <functional>
#include <memory>
#include <string>
#include "client.hpp"
#include "evalflags.hpp"
#include "lru2q.hpp"

namespace eppoclient {

/**
 * CacheKey for assignment logging.
 * Uses flag key and subject key to uniquely identify an assignment.
 */
struct AssignmentCacheKey {
    std::string flag;
    std::string subject;

    AssignmentCacheKey() = default;
    AssignmentCacheKey(const std::string& f, const std::string& s) : flag(f), subject(s) {}

    bool operator==(const AssignmentCacheKey& other) const {
        return flag == other.flag && subject == other.subject;
    }
};

/**
 * CacheValue for assignment logging.
 * Stores allocation and variation to detect changes.
 */
struct AssignmentCacheValue {
    std::string allocation;
    std::string variation;

    AssignmentCacheValue() = default;
    AssignmentCacheValue(const std::string& a, const std::string& v)
        : allocation(a), variation(v) {}

    bool operator==(const AssignmentCacheValue& other) const {
        return allocation == other.allocation && variation == other.variation;
    }

    bool operator!=(const AssignmentCacheValue& other) const { return !(*this == other); }
};

}  // namespace eppoclient

// Hash function for AssignmentCacheKey
namespace std {
template <>
struct hash<eppoclient::AssignmentCacheKey> {
    size_t operator()(const eppoclient::AssignmentCacheKey& key) const {
        size_t h1 = std::hash<std::string>{}(key.flag);
        size_t h2 = std::hash<std::string>{}(key.subject);
        return h1 ^ (h2 << 1);
    }
};
}  // namespace std

namespace eppoclient {

/**
 * LruAssignmentLogger wraps an AssignmentLogger and deduplicates assignment events
 * using an LRU 2Q cache.
 *
 * This logger caches recent assignments by (flag, subject) key and only logs when:
 * 1. The assignment is new (not in cache)
 * 2. The allocation or variation has changed
 *
 * This prevents duplicate logging when the same subject evaluates the same flag
 * multiple times with the same result.
 */
class LruAssignmentLogger : public AssignmentLogger {
private:
    cache::TwoQueueCache<AssignmentCacheKey, AssignmentCacheValue> cache_;
    std::shared_ptr<AssignmentLogger> inner_;

    /**
     * Determines whether an assignment should be logged based on cache state.
     *
     * @param key The cache key for the assignment
     * @param value The cache value for the assignment
     * @return true if the assignment should be logged, false otherwise
     */
    bool shouldLog(const AssignmentCacheKey& key, const AssignmentCacheValue& value);

public:
    /**
     * Creates a new LruAssignmentLogger.
     *
     * @param logger The inner logger to delegate to
     * @param cacheSize Maximum number of assignments to cache
     * @throws std::invalid_argument if cacheSize is invalid
     */
    LruAssignmentLogger(std::shared_ptr<AssignmentLogger> logger, size_t cacheSize);

    /**
     * Logs an assignment event, deduplicating based on the cache.
     *
     * @param event The assignment event to log
     */
    void logAssignment(const AssignmentEvent& event) override;
};

/**
 * Creates a new LruAssignmentLogger.
 *
 * @param logger The inner logger to delegate to
 * @param cacheSize Maximum number of assignments to cache
 * @return A shared pointer to the created logger
 * @throws std::invalid_argument if cacheSize is invalid
 */
std::shared_ptr<AssignmentLogger> NewLruAssignmentLogger(std::shared_ptr<AssignmentLogger> logger,
                                                         size_t cacheSize);

}  // namespace eppoclient

#endif  // LRU_ASSIGNMENT_LOGGER_HPP

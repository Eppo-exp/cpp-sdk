#ifndef LRU_BANDIT_LOGGER_HPP
#define LRU_BANDIT_LOGGER_HPP

#include <memory>
#include <string>
#include <functional>
#include "client.hpp"
#include "evalbandits.hpp"
#include "lru2q.hpp"

namespace eppoclient {

/**
 * CacheKey for bandit logging.
 * Uses flag key and subject key to uniquely identify a bandit action.
 */
struct BanditCacheKey {
    std::string flagKey;
    std::string subjectKey;

    BanditCacheKey() = default;
    BanditCacheKey(const std::string& f, const std::string& s)
        : flagKey(f), subjectKey(s) {}

    bool operator==(const BanditCacheKey& other) const {
        return flagKey == other.flagKey && subjectKey == other.subjectKey;
    }
};

/**
 * CacheValue for bandit logging.
 * Stores bandit key and action key to detect changes.
 */
struct BanditCacheValue {
    std::string banditKey;
    std::string actionKey;

    BanditCacheValue() = default;
    BanditCacheValue(const std::string& b, const std::string& a)
        : banditKey(b), actionKey(a) {}

    bool operator==(const BanditCacheValue& other) const {
        return banditKey == other.banditKey && actionKey == other.actionKey;
    }

    bool operator!=(const BanditCacheValue& other) const {
        return !(*this == other);
    }
};

} // namespace eppoclient

// Hash function for BanditCacheKey
namespace std {
    template<>
    struct hash<eppoclient::BanditCacheKey> {
        size_t operator()(const eppoclient::BanditCacheKey& key) const {
            size_t h1 = std::hash<std::string>{}(key.flagKey);
            size_t h2 = std::hash<std::string>{}(key.subjectKey);
            return h1 ^ (h2 << 1);
        }
    };
}

namespace eppoclient {

/**
 * LruBanditLogger wraps a BanditLogger and deduplicates bandit action events
 * using an LRU 2Q cache.
 *
 * This logger caches recent bandit actions by (flagKey, subjectKey) and only logs when:
 * 1. The action is new (not in cache)
 * 2. The bandit key or action key has changed
 *
 * This prevents duplicate logging when the same subject evaluates the same bandit
 * multiple times with the same result.
 */
class LruBanditLogger : public BanditLogger {
private:
    cache::TwoQueueCache<BanditCacheKey, BanditCacheValue> cache_;
    std::shared_ptr<BanditLogger> inner_;

    /**
     * Determines whether a bandit action should be logged based on cache state.
     *
     * @param key The cache key for the bandit action
     * @param value The cache value for the bandit action
     * @return true if the bandit action should be logged, false otherwise
     */
    bool shouldLog(const BanditCacheKey& key, const BanditCacheValue& value);

public:
    /**
     * Creates a new LruBanditLogger.
     *
     * @param logger The inner logger to delegate to (can be nullptr to disable logging)
     * @param cacheSize Maximum number of bandit actions to cache
     * @throws std::invalid_argument if cacheSize is invalid
     */
    LruBanditLogger(std::shared_ptr<BanditLogger> logger, size_t cacheSize);

    /**
     * Logs a bandit action event, deduplicating based on the cache.
     * If the inner logger is nullptr, this is a no-op.
     *
     * @param event The bandit event to log
     */
    void logBanditAction(const BanditEvent& event) override;
};

/**
 * Creates a new LruBanditLogger.
 *
 * @param logger The inner logger to delegate to (can be nullptr to disable logging)
 * @param cacheSize Maximum number of bandit actions to cache
 * @return A shared pointer to the created logger
 * @throws std::invalid_argument if cacheSize is invalid
 */
std::shared_ptr<BanditLogger> NewLruBanditLogger(
    std::shared_ptr<BanditLogger> logger,
    size_t cacheSize);

} // namespace eppoclient

#endif // LRU_BANDIT_LOGGER_HPP

#include "lru_bandit_logger.hpp"

namespace eppoclient {

LruBanditLogger::LruBanditLogger(
    std::shared_ptr<BanditLogger> logger,
    size_t cacheSize)
    : cache_(cacheSize), inner_(logger) {
    if (!logger) {
        throw std::invalid_argument("Error initializing bandit logger: inner logger cannot be null");
    }
}

void LruBanditLogger::logBanditAction(const BanditEvent& event) {
    BanditCacheKey key(event.flagKey, event.subject);
    BanditCacheValue value(event.banditKey, event.action);

    auto [previousValue, recentlyLogged] = cache_.Get(key);

    if (!recentlyLogged || previousValue != value) {
        // Log the bandit action first, then add to cache
        // This ensures that if logging throws an exception,
        // we don't cache it (matching Go behavior)
        inner_->logBanditAction(event);

        // Adding to cache after LogBanditAction returned in case it panics
        cache_.Add(key, value);
    }
}

std::shared_ptr<BanditLogger> NewLruBanditLogger(
    std::shared_ptr<BanditLogger> logger,
    size_t cacheSize) {
    return std::make_shared<LruBanditLogger>(logger, cacheSize);
}

} // namespace eppoclient

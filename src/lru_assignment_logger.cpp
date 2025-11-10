#include "lru_assignment_logger.hpp"
#include <stdexcept>

namespace eppoclient {

LruAssignmentLogger::LruAssignmentLogger(
    std::shared_ptr<AssignmentLogger> logger,
    size_t cacheSize)
    : cache_(cacheSize), inner_(logger) {
    if (!logger) {
        throw std::invalid_argument("Error initializing assignment logger: inner logger cannot be null");
    }
}

void LruAssignmentLogger::logAssignment(const AssignmentEvent& event) {
    AssignmentCacheKey key(event.featureFlag, event.subject);
    AssignmentCacheValue value(event.allocation, event.variation);

    auto [previousValue, recentlyLogged] = cache_.Get(key);

    if (!recentlyLogged || previousValue != value) {
        // Log the assignment first, then add to cache
        // This ensures that if logging throws an exception,
        // we don't cache it (matching Go behavior)
        inner_->logAssignment(event);

        // Adding to cache after LogAssignment returned in case it panics
        cache_.Add(key, value);
    }
}

std::shared_ptr<AssignmentLogger> NewLruAssignmentLogger(
    std::shared_ptr<AssignmentLogger> logger,
    size_t cacheSize) {
    return std::make_shared<LruAssignmentLogger>(logger, cacheSize);
}

} // namespace eppoclient

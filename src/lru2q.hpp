#ifndef LRU2Q_HPP
#define LRU2Q_HPP

#include <list>
#include <unordered_map>
#include <utility>
#include <cassert>

namespace eppoclient {
namespace cache {

/**
 * TwoQueueCache implements the 2Q cache algorithm.
 *
 * The 2Q algorithm uses two queues to track frequently and recently used items:
 * - recentQueue: Items that have been accessed once (FIFO)
 * - frequentQueue: Items that have been accessed multiple times (LRU)
 *
 * This provides better performance than a simple LRU for many workloads.
 */
template<typename Key, typename Value>
class TwoQueueCache {
private:
    // Node structure for doubly linked list
    struct Node {
        Key key;
        Value value;

        Node(const Key& k, const Value& v) : key(k), value(v) {}
    };

    // Recent queue (FIFO) - items accessed once
    std::list<Node> recentQueue_;
    std::unordered_map<Key, typename std::list<Node>::iterator> recentIndex_;

    // Frequent queue (LRU) - items accessed multiple times
    std::list<Node> frequentQueue_;
    std::unordered_map<Key, typename std::list<Node>::iterator> frequentIndex_;

    // Ghost entries - tracks recently evicted keys from recent queue
    std::list<Key> ghostQueue_;
    std::unordered_map<Key, typename std::list<Key>::iterator> ghostIndex_;

    size_t size_;
    size_t recentSize_;
    size_t ghostSize_;

    void evictRecent() {
        if (recentQueue_.empty()) {
            return;
        }

        // Move oldest from recent queue to ghost queue
        auto& evicted = recentQueue_.back();
        Key key = evicted.key;

        recentIndex_.erase(key);
        recentQueue_.pop_back();

        // Add to ghost queue
        ghostQueue_.push_front(key);
        ghostIndex_[key] = ghostQueue_.begin();

        // Evict from ghost if needed
        if (ghostQueue_.size() > ghostSize_) {
            Key ghostKey = ghostQueue_.back();
            ghostIndex_.erase(ghostKey);
            ghostQueue_.pop_back();
        }
    }

    void evictFrequent() {
        if (frequentQueue_.empty()) {
            return;
        }

        // Remove least recently used from frequent queue
        auto& evicted = frequentQueue_.back();
        frequentIndex_.erase(evicted.key);
        frequentQueue_.pop_back();
    }

public:
    /**
     * Creates a new TwoQueueCache with the given size.
     *
     * @param size Maximum number of items to cache (must be positive)
     * @pre size > 0
     */
    explicit TwoQueueCache(size_t size) {
        assert(size > 0 && "cache size must be positive");

        size_ = size;
        // Recent queue gets 25% of cache, frequent gets 75%
        recentSize_ = size / 4;
        if (recentSize_ < 1) {
            recentSize_ = 1;
        }
        // Ghost queue is same size as recent queue
        ghostSize_ = recentSize_;
    }

    /**
     * Gets a value from the cache.
     *
     * @param key The key to look up
     * @return A pair of (value, found) where found indicates if the key was present
     */
    std::pair<Value, bool> Get(const Key& key) {
        // Check frequent queue first
        auto freqIt = frequentIndex_.find(key);
        if (freqIt != frequentIndex_.end()) {
            // Move to front (most recently used)
            auto nodeIt = freqIt->second;
            Value value = nodeIt->value;
            frequentQueue_.splice(frequentQueue_.begin(), frequentQueue_, nodeIt);
            return {value, true};
        }

        // Check recent queue
        auto recentIt = recentIndex_.find(key);
        if (recentIt != recentIndex_.end()) {
            // Promote to frequent queue
            auto nodeIt = recentIt->second;
            Key k = nodeIt->key;
            Value value = nodeIt->value;

            recentIndex_.erase(key);
            recentQueue_.erase(nodeIt);

            // Ensure space in frequent queue
            if (frequentQueue_.size() >= size_ - recentSize_) {
                evictFrequent();
            }

            frequentQueue_.emplace_front(k, value);
            frequentIndex_[k] = frequentQueue_.begin();

            return {value, true};
        }

        // Not found
        return {Value{}, false};
    }

    /**
     * Adds a value to the cache.
     *
     * @param key The key to add
     * @param value The value to add
     */
    void Add(const Key& key, const Value& value) {
        // Check if already in frequent queue
        auto freqIt = frequentIndex_.find(key);
        if (freqIt != frequentIndex_.end()) {
            // Update and move to front
            auto nodeIt = freqIt->second;
            nodeIt->value = value;
            frequentQueue_.splice(frequentQueue_.begin(), frequentQueue_, nodeIt);
            return;
        }

        // Check if already in recent queue
        auto recentIt = recentIndex_.find(key);
        if (recentIt != recentIndex_.end()) {
            // Update value
            recentIt->second->value = value;
            return;
        }

        // Check if in ghost queue (was recently evicted)
        auto ghostIt = ghostIndex_.find(key);
        if (ghostIt != ghostIndex_.end()) {
            // Add directly to frequent queue
            ghostIndex_.erase(key);
            ghostQueue_.erase(ghostIt->second);

            // Ensure space in frequent queue
            if (frequentQueue_.size() >= size_ - recentSize_) {
                evictFrequent();
            }

            frequentQueue_.emplace_front(key, value);
            frequentIndex_[key] = frequentQueue_.begin();
            return;
        }

        // New item - add to recent queue
        if (recentQueue_.size() >= recentSize_) {
            evictRecent();
        }

        recentQueue_.emplace_front(key, value);
        recentIndex_[key] = recentQueue_.begin();
    }

    /**
     * Gets the current number of items in the cache.
     */
    size_t Len() const {
        return recentQueue_.size() + frequentQueue_.size();
    }

    /**
     * Clears all items from the cache.
     */
    void Clear() {
        recentQueue_.clear();
        recentIndex_.clear();
        frequentQueue_.clear();
        frequentIndex_.clear();
        ghostQueue_.clear();
        ghostIndex_.clear();
    }
};

} // namespace cache
} // namespace eppoclient

#endif // LRU2Q_HPP

#ifndef CONFIGURATION_STORE_H
#define CONFIGURATION_STORE_H

#include <atomic>
#include <memory>
#include <mutex>
#include <condition_variable>
#include "configuration.hpp"

namespace eppoclient {

/**
 * ConfigurationStore is a thread-safe in-memory storage. It stores
 * the currently active configuration and provides access to multiple
 * readers (e.g., flag evaluation) and writers (e.g.,
 * configuration requestor).
 */
class ConfigurationStore {
public:
    ConfigurationStore();
    explicit ConfigurationStore(const Configuration& config);

    ~ConfigurationStore() = default;

    // Delete copy constructor and assignment operator to prevent copying
    ConfigurationStore(const ConfigurationStore&) = delete;
    ConfigurationStore& operator=(const ConfigurationStore&) = delete;

    /**
     * Returns a snapshot of the currently active configuration.
     * If no configuration has been set, returns an empty configuration.
     */
    Configuration getConfiguration() const;

    /**
     * Sets the configuration and marks the store as initialized.
     * Thread-safe operation that can be called from multiple threads.
     */
    void setConfiguration(const Configuration& config);

    /**
     * Blocks until the configuration store is initialized.
     * Returns immediately if already initialized.
     */
    void waitForInitialization() const;

    /**
     * Checks if the configuration store has been initialized.
     */
    bool isInitialized() const;

private:
    // Current configuration protected by mutex
    // Using shared_ptr for memory management
    std::shared_ptr<Configuration> configuration_;
    mutable std::mutex configMutex_;

    // Flag to track initialization state
    std::atomic<bool> isInitialized_;

    // Condition variable and mutex for waiting on initialization
    mutable std::condition_variable initCV_;
    mutable std::mutex initMutex_;

    /**
     * Set the initialized flag to true and notify any waiting threads.
     */
    void setInitialized();
};

} // namespace eppoclient

#endif // CONFIGURATION_STORE_H

#ifndef CONFIGURATION_STORE_HPP
#define CONFIGURATION_STORE_HPP

#include <memory>
#include <mutex>
#include "configuration.hpp"

namespace eppoclient {

/**
 * ConfigurationStore is a thread-safe in-memory storage. It stores
 * the currently active configuration and provides access to multiple
 * readers (e.g., flag/bandit evaluation) and writers (e.g.,
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
     * Sets the configuration.
     * Thread-safe operation that can be called from multiple threads.
     */
    void setConfiguration(const Configuration& config);

private:
    // Current configuration protected by mutex
    // Using shared_ptr for memory management
    std::shared_ptr<Configuration> configuration_;
    mutable std::mutex configMutex_;
};

} // namespace eppoclient

#endif // CONFIGURATION_STORE_H

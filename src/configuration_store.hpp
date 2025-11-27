#ifndef CONFIGURATION_STORE_HPP
#define CONFIGURATION_STORE_HPP

#include <memory>
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
    explicit ConfigurationStore(std::shared_ptr<const Configuration> config);
    explicit ConfigurationStore(Configuration config);

    ~ConfigurationStore() = default;

    // Delete copy constructor and assignment operator to prevent copying
    ConfigurationStore(const ConfigurationStore&) = delete;
    ConfigurationStore& operator=(const ConfigurationStore&) = delete;

    /**
     * Returns a shared pointer to the currently active configuration.
     * If no configuration has been set, returns an empty configuration.
     *
     * Thread-safe.
     */
    std::shared_ptr<const Configuration> getConfiguration() const;

    /**
     * Sets the configuration from a shared pointer.
     *
     * Thread-safe.
     */
    void setConfiguration(std::shared_ptr<const Configuration> config);

    /**
     * Sets the configuration from a Configuration object.
     *
     * Thread-safe.
     */
    void setConfiguration(Configuration config);

private:
    // Current configuration accessed atomically for thread safety
    std::shared_ptr<const Configuration> configuration_;
};

}  // namespace eppoclient

#endif  // CONFIGURATION_STORE_H

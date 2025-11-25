#ifndef CONFIGURATION_STORE_HPP
#define CONFIGURATION_STORE_HPP

#include <optional>
#include "configuration.hpp"

namespace eppoclient {

/**
 * ConfigurationStore is an in-memory storage for the currently active configuration.
 * It provides access to readers (e.g., flag/bandit evaluation) and writers
 * (e.g., configuration requestor).
 *
 * Note: This class is not thread-safe. If you need to access it from multiple threads,
 * the caller is responsible for providing appropriate synchronization.
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
     * Returns a copy of the currently active configuration.
     * If no configuration has been set, returns an empty configuration.
     */
    Configuration getConfiguration() const;

    /**
     * Sets the configuration.
     */
    void setConfiguration(const Configuration& config);

private:
    std::optional<Configuration> configuration_;
};

}  // namespace eppoclient

#endif  // CONFIGURATION_STORE_H

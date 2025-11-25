#ifndef CONFIGURATION_STORE_HPP
#define CONFIGURATION_STORE_HPP

#include <memory>
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
     * Returns a shared pointer to the currently active configuration.
     * If no configuration has been set, returns nullptr.
     *
     * The returned shared_ptr provides thread-safe reference counting
     * and ensures the configuration remains valid for the lifetime of the pointer.
     */
    std::shared_ptr<const Configuration> getConfiguration() const;

    /**
     * Sets the configuration.
     */
    void setConfiguration(const Configuration& config);

private:
    std::shared_ptr<const Configuration> configuration_;
};

}  // namespace eppoclient

#endif  // CONFIGURATION_STORE_H

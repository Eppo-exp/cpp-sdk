#include "configuration_store.hpp"

namespace eppoclient {

ConfigurationStore::ConfigurationStore()
    : configuration_(nullptr) {
}

ConfigurationStore::ConfigurationStore(const Configuration& config)
    : configuration_(nullptr) {
    setConfiguration(config);
}

Configuration ConfigurationStore::getConfiguration() const {
    // Lock and get a copy of the shared_ptr
    std::lock_guard<std::mutex> lock(configMutex_);

    if (configuration_ != nullptr) {
        // Return a copy of the configuration
        return *configuration_;
    } else {
        // Return an empty configuration
        return Configuration();
    }
}

void ConfigurationStore::setConfiguration(const Configuration& config) {
    // Create a new shared_ptr with a copy of the configuration
    auto newConfig = std::make_shared<Configuration>(config);

    // Precompute any derived data in the configuration
    newConfig->precompute();

    // Lock and store the new configuration
    std::lock_guard<std::mutex> lock(configMutex_);
    configuration_ = newConfig;
}

} // namespace eppoclient

#include "configuration_store.hpp"

namespace eppoclient {

ConfigurationStore::ConfigurationStore()
    : configuration_(std::nullopt) {
}

ConfigurationStore::ConfigurationStore(const Configuration& config)
    : configuration_(std::nullopt) {
    setConfiguration(config);
}

Configuration ConfigurationStore::getConfiguration() const {
    if (configuration_.has_value()) {
        return *configuration_;
    } else {
        return Configuration();
    }
}

void ConfigurationStore::setConfiguration(const Configuration& config) {
    // Make a copy of the configuration
    Configuration newConfig = config;

    // Precompute any derived data in the configuration
    newConfig.precompute();

    // Store the configuration
    configuration_ = newConfig;
}

} // namespace eppoclient

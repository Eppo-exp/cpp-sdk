#include "configuration_store.hpp"

namespace eppoclient {

ConfigurationStore::ConfigurationStore() : configuration_(nullptr) {}

ConfigurationStore::ConfigurationStore(const Configuration& config) : configuration_(nullptr) {
    setConfiguration(config);
}

std::shared_ptr<const Configuration> ConfigurationStore::getConfiguration() const {
    return configuration_;
}

void ConfigurationStore::setConfiguration(const Configuration& config) {
    // Make a copy of the configuration
    Configuration newConfig = config;

    // Precompute any derived data in the configuration
    newConfig.precompute();

    // Store the configuration as a shared_ptr
    configuration_ = std::make_shared<const Configuration>(std::move(newConfig));
}

}  // namespace eppoclient

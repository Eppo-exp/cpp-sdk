#include "configuration_store.hpp"

namespace eppoclient {

ConfigurationStore::ConfigurationStore()
    : configuration_(std::make_shared<const Configuration>()) {}

ConfigurationStore::ConfigurationStore(std::shared_ptr<const Configuration> config)
    : configuration_(config ? std::move(config) : std::make_shared<const Configuration>()) {}

ConfigurationStore::ConfigurationStore(Configuration config)
    : configuration_(std::make_shared<const Configuration>(std::move(config))) {}

std::shared_ptr<const Configuration> ConfigurationStore::getConfiguration() const {
    return std::atomic_load(&configuration_);
}

void ConfigurationStore::setConfiguration(std::shared_ptr<const Configuration> config) {
    if (!config) {
        config = std::make_shared<const Configuration>();
    }

    std::atomic_store(&configuration_, std::move(config));
}

void ConfigurationStore::setConfiguration(Configuration config) {
    setConfiguration(std::make_shared<const Configuration>(std::move(config)));
}

}  // namespace eppoclient

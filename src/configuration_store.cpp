#include "configuration_store.hpp"

namespace eppoclient {

ConfigurationStore::ConfigurationStore()
    : configuration_(nullptr), isInitialized_(false) {
}

ConfigurationStore::ConfigurationStore(const Configuration& config)
    : configuration_(nullptr), isInitialized_(false) {
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
    {
        std::lock_guard<std::mutex> lock(configMutex_);
        configuration_ = newConfig;
    }

    // Mark as initialized and notify waiters
    setInitialized();
}

void ConfigurationStore::setInitialized() {
    // Use compare_exchange to ensure we only notify once
    bool expected = false;
    if (isInitialized_.compare_exchange_strong(expected, true)) {
        // Lock and notify all waiting threads
        {
            std::lock_guard<std::mutex> lock(initMutex_);
        }
        initCV_.notify_all();
    }
}

void ConfigurationStore::waitForInitialization() const {
    // Quick check without locking
    if (isInitialized_.load()) {
        return;
    }

    // Wait with condition variable
    std::unique_lock<std::mutex> lock(initMutex_);
    initCV_.wait(lock, [this] {
        return isInitialized_.load();
    });
}

bool ConfigurationStore::isInitialized() const {
    return isInitialized_.load();
}

} // namespace eppoclient

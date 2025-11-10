#include "configuration.hpp"

namespace eppoclient {

Configuration::Configuration(const ConfigResponse& response)
    : flags_(response) {
    precompute();
}

void Configuration::precompute() {
    // Precompute flag configurations
    flags_.precompute();
}

const FlagConfiguration* Configuration::getFlagConfiguration(const std::string& key) const {
    auto it = flags_.flags.find(key);
    if (it == flags_.flags.end()) {
        return nullptr;
    }
    return &(it->second);
}

} // namespace eppoclient

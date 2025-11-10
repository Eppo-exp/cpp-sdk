#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <string>
#include "config_response.hpp"

namespace eppoclient {

/**
 * Configuration holds the flag configuration data.
 */
class Configuration {
public:
    Configuration() = default;
    explicit Configuration(const ConfigResponse& response);
    ~Configuration() = default;

    // Copy constructor and assignment operator
    Configuration(const Configuration& other) = default;
    Configuration& operator=(const Configuration& other) = default;

    /**
     * Precompute derived data structures for faster access during evaluation.
     */
    void precompute();

    /**
     * Get flag configuration by key.
     * Returns nullptr if not found.
     */
    const FlagConfiguration* getFlagConfiguration(const std::string& key) const;

private:
    ConfigResponse flags_;
};

} // namespace eppoclient

#endif // CONFIGURATION_H

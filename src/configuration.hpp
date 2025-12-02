#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include <string>
#include "bandit_model.hpp"
#include "config_response.hpp"

namespace eppoclient {

/**
 * Configuration holds the flag and bandit configuration data.
 * This is a stub implementation that will be expanded later.
 */
class Configuration {
public:
    Configuration() = default;
    explicit Configuration(ConfigResponse response);
    Configuration(ConfigResponse flagsResponse, BanditResponse banditsResponse);
    ~Configuration() = default;

    // Copy constructor and assignment operator
    Configuration(const Configuration& other) = default;
    Configuration& operator=(const Configuration& other) = default;

    // Move constructor and assignment operator
    Configuration(Configuration&& other) = default;
    Configuration& operator=(Configuration&& other) = default;

    /**
     * Get bandit variation for a given flag key and variation value.
     * Returns true if found, false otherwise.
     */
    bool getBanditVariant(const std::string& flagKey, const std::string& variation,
                          BanditVariation& result) const;

    /**
     * Get flag configuration by key.
     * Returns nullptr if not found.
     */
    const FlagConfiguration* getFlagConfiguration(const std::string& key) const;

    /**
     * Get bandit configuration by key.
     * Returns nullptr if not found.
     */
    const BanditConfiguration* getBanditConfiguration(const std::string& key) const;

private:
    ConfigResponse flags_;
    BanditResponse bandits_;

    // Flag key -> variation value -> banditVariation
    // This is cached from bandits response for easier access in evaluation
    std::map<std::string, std::map<std::string, BanditVariation>> banditFlagAssociations_;
};

/**
 * Parse complete configuration from JSON strings with simplified error handling.
 *
 * This is a convenience wrapper around parseConfigResponse() and parseBanditResponse()
 * that provides a simpler API for common use cases.
 *
 * @param flagConfigJson JSON string containing flag configuration
 * @param banditModelsJson JSON string containing bandit models (optional - pass empty string to
 * skip)
 * @param error Output parameter that will contain error message if parsing fails
 * @return Configuration object. Check error parameter to see if parsing was successful.
 *
 * Example usage:
 * @code
 *   std::string error;
 *   Configuration config = parseConfiguration(flagsJson, banditsJson, error);
 *   if (!error.empty()) {
 *       std::cerr << "Configuration parsing error: " << error << std::endl;
 *       return 1;
 *   }
 * @endcode
 */
Configuration parseConfiguration(const std::string& flagConfigJson,
                                 const std::string& banditModelsJson, std::string& error);

/**
 * Parse flag configuration only (without bandit models) with simplified error handling.
 *
 * @param flagConfigJson JSON string containing flag configuration
 * @param error Output parameter that will contain error message if parsing fails
 * @return Configuration object. Check error parameter to see if parsing was successful.
 */
Configuration parseConfiguration(const std::string& flagConfigJson, std::string& error);

}  // namespace eppoclient

#endif  // CONFIGURATION_H

#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include <string>
#include "bandit_model.hpp"
#include "config_response.hpp"
#include "parse_result.hpp"

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
 * Parse complete configuration from JSON strings.
 *
 * This is a convenience wrapper around parseConfigResponse() and parseBanditResponse()
 * that provides a simpler API for common use cases.
 *
 * @param flagConfigJson JSON string containing flag configuration
 * @param banditModelsJson JSON string containing bandit models (optional - pass empty string to
 * skip)
 * @return ParseResult containing Configuration object and any errors encountered during parsing
 *
 * Example usage:
 * @code
 *   // Parse flags only
 *   auto result = parseConfiguration(flagsJson);
 *
 *   // Parse flags and bandits
 *   auto result = parseConfiguration(flagsJson, banditsJson);
 *
 *   if (!result.hasValue()) {
 *       std::cerr << "Configuration parsing failed:" << std::endl;
 *       for (const auto& error : result.errors) {
 *           std::cerr << "  - " << error << std::endl;
 *       }
 *       return 1;
 *   }
 *   Configuration config = std::move(*result.value);
 * @endcode
 */
ParseResult<Configuration> parseConfiguration(const std::string& flagConfigJson,
                                              const std::string& banditModelsJson = "");

}  // namespace eppoclient

#endif  // CONFIGURATION_H

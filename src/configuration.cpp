#include "configuration.hpp"

namespace eppoclient {

Configuration::Configuration(ConfigResponse response)
    : Configuration(std::move(response), BanditResponse{}) {}

Configuration::Configuration(ConfigResponse flagsResponse, BanditResponse banditsResponse)
    : flags_(std::move(flagsResponse)), bandits_(std::move(banditsResponse)) {
    // Precompute flag configurations
    flags_.precompute();

    // Build banditFlagAssociations map for O(1) lookup during evaluation
    // Iterate through all bandit variations in the config response
    for (const auto& [key, banditVariations] : flags_.bandits) {
        // Each key maps to a vector of BanditVariation objects
        for (const auto& banditVariation : banditVariations) {
            const std::string& flagKey = banditVariation.flagKey;
            const std::string& variationValue = banditVariation.variationValue;

            // Create or get the map for this flag key
            auto& byVariation = banditFlagAssociations_[flagKey];

            // Store the bandit variation indexed by variation value
            byVariation[variationValue] = banditVariation;
        }
    }
}

bool Configuration::getBanditVariant(const std::string& flagKey, const std::string& variation,
                                     BanditVariation& result) const {
    auto flagIt = banditFlagAssociations_.find(flagKey);
    if (flagIt == banditFlagAssociations_.end()) {
        return false;
    }

    const auto& byVariation = flagIt->second;
    auto variationIt = byVariation.find(variation);
    if (variationIt == byVariation.end()) {
        return false;
    }

    result = variationIt->second;
    return true;
}

const FlagConfiguration* Configuration::getFlagConfiguration(const std::string& key) const {
    auto it = flags_.flags.find(key);
    if (it == flags_.flags.end()) {
        return nullptr;
    }
    return &(it->second);
}

const BanditConfiguration* Configuration::getBanditConfiguration(const std::string& key) const {
    auto it = bandits_.bandits.find(key);
    if (it == bandits_.bandits.end()) {
        return nullptr;
    }
    return &(it->second);
}

Configuration parseConfiguration(const std::string& flagConfigJson,
                                 const std::string& banditModelsJson, std::string& error) {
    error.clear();

    // Parse flag configuration
    std::string flagError;
    ConfigResponse flagConfig = internal::parseConfigResponse(flagConfigJson, flagError);
    if (!flagError.empty()) {
        error = "Configuration parsing error: " + flagError;
        return Configuration();
    }

    // Parse bandit models if provided
    BanditResponse banditModels;
    if (!banditModelsJson.empty()) {
        std::string banditError;
        banditModels = internal::parseBanditResponse(banditModelsJson, banditError);
        if (!banditError.empty()) {
            error = "Bandit models parsing error: " + banditError;
            return Configuration();
        }
    }

    return Configuration(std::move(flagConfig), std::move(banditModels));
}

Configuration parseConfiguration(const std::string& flagConfigJson, std::string& error) {
    return parseConfiguration(flagConfigJson, "", error);
}

}  // namespace eppoclient

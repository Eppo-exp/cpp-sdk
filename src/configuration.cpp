#include "configuration.hpp"
#include <nlohmann/json.hpp>

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

ParseResult<Configuration> parseConfiguration(const std::string& flagConfigJson,
                                              const std::string& banditModelsJson) {
    ParseResult<Configuration> result;

    // Parse flag configuration JSON
    nlohmann::json flagsJson = nlohmann::json::parse(flagConfigJson, nullptr, false);
    if (flagsJson.is_discarded()) {
        result.errors.push_back("Failed to parse flag configuration JSON: invalid JSON");
        return result;
    }

    // Parse the flag configuration
    auto flagResult = parseConfigResponse(flagsJson);
    if (!flagResult.hasValue()) {
        result.errors.push_back("Failed to parse flag configuration");
        if (flagResult.hasErrors()) {
            for (const auto& err : flagResult.errors) {
                result.errors.push_back("  " + err);
            }
        }
        return result;
    }

    // Optionally parse bandit models if provided
    BanditResponse banditModels;
    if (!banditModelsJson.empty()) {
        nlohmann::json banditsJson = nlohmann::json::parse(banditModelsJson, nullptr, false);
        if (banditsJson.is_discarded()) {
            result.errors.push_back("Failed to parse bandit models JSON: invalid JSON");
            return result;
        }

        auto banditResult = parseBanditResponse(banditsJson);
        if (!banditResult.hasValue()) {
            result.errors.push_back("Failed to parse bandit models");
            if (banditResult.hasErrors()) {
                for (const auto& err : banditResult.errors) {
                    result.errors.push_back("  " + err);
                }
            }
            return result;
        }

        // Collect any warnings from bandit parsing
        if (banditResult.hasErrors()) {
            for (const auto& err : banditResult.errors) {
                result.errors.push_back("Bandit warning: " + err);
            }
        }

        banditModels = *banditResult.value;
    }

    // Collect any warnings from flag parsing
    if (flagResult.hasErrors()) {
        for (const auto& err : flagResult.errors) {
            result.errors.push_back("Flag warning: " + err);
        }
    }

    // Set the configuration value
    result.value = Configuration(std::move(*flagResult.value), std::move(banditModels));
    return result;
}

ParseResult<Configuration> parseConfiguration(const std::string& flagConfigJson) {
    return parseConfiguration(flagConfigJson, "");
}

}  // namespace eppoclient

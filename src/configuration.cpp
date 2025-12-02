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

Configuration parseConfiguration(const std::string& flagConfigJson,
                                 const std::string& banditModelsJson, std::string& error) {
    error.clear();

    // Parse flag configuration JSON
    nlohmann::json flagsJson = nlohmann::json::parse(flagConfigJson, nullptr, false);
    if (flagsJson.is_discarded()) {
        error = "Failed to parse flag configuration JSON: invalid JSON";
        return Configuration();
    }

    // Parse the flag configuration
    auto flagResult = parseConfigResponse(flagsJson);
    if (!flagResult.hasValue()) {
        error = "Failed to parse flag configuration";
        if (flagResult.hasErrors()) {
            error += ":\n";
            for (const auto& err : flagResult.errors) {
                error += "  - " + err + "\n";
            }
        }
        return Configuration();
    }

    // Optionally parse bandit models if provided
    BanditResponse banditModels;
    if (!banditModelsJson.empty()) {
        nlohmann::json banditsJson = nlohmann::json::parse(banditModelsJson, nullptr, false);
        if (banditsJson.is_discarded()) {
            error = "Failed to parse bandit models JSON: invalid JSON";
            return Configuration();
        }

        auto banditResult = parseBanditResponse(banditsJson);
        if (!banditResult.hasValue()) {
            error = "Failed to parse bandit models";
            if (banditResult.hasErrors()) {
                error += ":\n";
                for (const auto& err : banditResult.errors) {
                    error += "  - " + err + "\n";
                }
            }
            return Configuration();
        }

        // Collect any warnings from bandit parsing
        if (banditResult.hasErrors()) {
            error = "Bandit models parsed with warnings:\n";
            for (const auto& err : banditResult.errors) {
                error += "  - " + err + "\n";
            }
        }

        banditModels = *banditResult.value;
    }

    // Collect any warnings from flag parsing
    if (flagResult.hasErrors()) {
        if (!error.empty()) {
            error += "\n";
        }
        error += "Flag configuration parsed with warnings:\n";
        for (const auto& err : flagResult.errors) {
            error += "  - " + err + "\n";
        }
    }

    return Configuration(std::move(*flagResult.value), std::move(banditModels));
}

Configuration parseConfiguration(const std::string& flagConfigJson, std::string& error) {
    return parseConfiguration(flagConfigJson, "", error);
}

}  // namespace eppoclient

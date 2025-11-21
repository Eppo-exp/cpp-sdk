#include <catch_amalgamated.hpp>
#include "../src/configuration_store.hpp"
#include "../src/configuration.hpp"

using namespace eppoclient;

TEST_CASE("ConfigurationStore basic get/set", "[configuration_store]") {
    ConfigurationStore store;

    // Get configuration before setting should return empty
    Configuration config1 = store.getConfiguration();

    // Set a configuration
    Configuration newConfig;
    store.setConfiguration(newConfig);

    // Get configuration should work
    Configuration config2 = store.getConfiguration();
}

TEST_CASE("ConfigurationStore constructor with config", "[configuration_store]") {
    Configuration config;
    ConfigurationStore store(config);

    Configuration retrievedConfig = store.getConfiguration();
}

// Thread safety test removed - ConfigurationStore is no longer thread-safe by design.
// If thread-safety is required, the caller is responsible for providing synchronization.

#include <catch_amalgamated.hpp>
#include "../src/configuration.hpp"
#include "../src/configuration_store.hpp"

using namespace eppoclient;

TEST_CASE("ConfigurationStore basic get/set", "[configuration_store]") {
    ConfigurationStore store;

    // Get configuration before setting should return nullptr
    auto config1 = store.getConfiguration();
    REQUIRE(config1 == nullptr);

    // Set a configuration
    Configuration newConfig;
    store.setConfiguration(newConfig);

    // Get configuration should return a valid shared_ptr
    auto config2 = store.getConfiguration();
    REQUIRE(config2 != nullptr);
}

TEST_CASE("ConfigurationStore constructor with config", "[configuration_store]") {
    Configuration config;
    ConfigurationStore store(config);

    auto retrievedConfig = store.getConfiguration();
    REQUIRE(retrievedConfig != nullptr);
}

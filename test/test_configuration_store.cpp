#include <catch_amalgamated.hpp>
#include "../src/configuration_store.hpp"
#include "../src/configuration.hpp"
#include <thread>

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

TEST_CASE("ConfigurationStore thread safety", "[configuration_store]") {
    ConfigurationStore store;
    const int numThreads = 10;
    const int numIterations = 100;

    // Multiple threads reading and writing
    std::vector<std::thread> threads;

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&store]() {
            for (int j = 0; j < numIterations; ++j) {
                Configuration config;
                store.setConfiguration(config);
                Configuration retrieved = store.getConfiguration();
                (void)retrieved; // Suppress unused variable warning
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }
}

#include <catch_amalgamated.hpp>
#include "../src/configuration_store.hpp"
#include "../src/configuration.hpp"
#include <thread>
#include <chrono>

using namespace eppoclient;

TEST_CASE("ConfigurationStore basic get/set", "[configuration_store]") {
    ConfigurationStore store;

    // Initially should not be initialized
    CHECK(!store.isInitialized());

    // Get configuration before setting should return empty
    Configuration config1 = store.getConfiguration();

    // Set a configuration
    Configuration newConfig;
    store.setConfiguration(newConfig);

    // Should be initialized now
    CHECK(store.isInitialized());

    // Get configuration should work
    Configuration config2 = store.getConfiguration();
}

TEST_CASE("ConfigurationStore constructor with config", "[configuration_store]") {
    Configuration config;
    ConfigurationStore store(config);

    // Should be initialized immediately
    CHECK(store.isInitialized());

    Configuration retrievedConfig = store.getConfiguration();
}

TEST_CASE("ConfigurationStore wait for initialization", "[configuration_store]") {
    ConfigurationStore store;
    bool threadCompleted = false;

    // Start a thread that waits for initialization
    std::thread waiter([&store, &threadCompleted]() {
        store.waitForInitialization();
        threadCompleted = true;
    });

    // Give the thread time to start waiting
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Thread should not have completed yet
    CHECK(!threadCompleted);

    // Initialize the store
    Configuration config;
    store.setConfiguration(config);

    // Wait for the thread to complete
    waiter.join();

    // Thread should have completed
    CHECK(threadCompleted);
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

    CHECK(store.isInitialized());
}

#include <catch_amalgamated.hpp>
#include "../src/configuration.hpp"
#include <nlohmann/json.hpp>

using namespace eppoclient;
using json = nlohmann::json;

TEST_CASE("Configuration getFlagConfiguration", "[configuration]") {
    Configuration config;

    const FlagConfiguration* flag = config.getFlagConfiguration("test-flag");

    // Should return nullptr since we haven't set any configuration
    CHECK(flag == nullptr);
}

TEST_CASE("Configuration precompute", "[configuration]") {
    Configuration config;

    // Should not crash when called on empty configuration
    config.precompute();
}

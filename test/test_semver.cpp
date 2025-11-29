#include <catch_amalgamated.hpp>
#include "../src/config_response.hpp"
#include "../src/rules.hpp"

using namespace eppoclient;
using namespace eppoclient::internal;

TEST_CASE("SemVer: Version 2.0.0 > 1.5.0", "[semver]") {
    Condition condition;
    condition.op = Operator::GT;
    condition.attribute = "app_version";
    condition.value = "1.5.0";
    condition.precompute();

    Attributes attributes;
    attributes["app_version"] = std::string("2.0.0");

    bool result = conditionMatches(condition, attributes, nullptr);
    CHECK(result == true);
}

TEST_CASE("SemVer: Version 1.2.3 < 1.5.0", "[semver]") {
    Condition condition;
    condition.op = Operator::LT;
    condition.attribute = "app_version";
    condition.value = "1.5.0";
    condition.precompute();

    Attributes attributes;
    attributes["app_version"] = std::string("1.2.3");

    bool result = conditionMatches(condition, attributes, nullptr);
    CHECK(result == true);
}

TEST_CASE("SemVer: Version 1.5.0 >= 1.5.0", "[semver]") {
    Condition condition;
    condition.op = Operator::GTE;
    condition.attribute = "app_version";
    condition.value = "1.5.0";
    condition.precompute();

    Attributes attributes;
    attributes["app_version"] = std::string("1.5.0");

    bool result = conditionMatches(condition, attributes, nullptr);
    CHECK(result == true);
}

TEST_CASE("SemVer: Version 1.4.9 <= 1.5.0", "[semver]") {
    Condition condition;
    condition.op = Operator::LTE;
    condition.attribute = "app_version";
    condition.value = "1.5.0";
    condition.precompute();

    Attributes attributes;
    attributes["app_version"] = std::string("1.4.9");

    bool result = conditionMatches(condition, attributes, nullptr);
    CHECK(result == true);
}

TEST_CASE("Numeric fallback when not a valid semver", "[semver]") {
    Condition condition;
    condition.op = Operator::GT;
    condition.attribute = "score";
    condition.value = 100;
    condition.precompute();

    Attributes attributes;
    attributes["score"] = 150.0;

    bool result = conditionMatches(condition, attributes, nullptr);
    CHECK(result == true);
}

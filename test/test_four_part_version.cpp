#include <catch_amalgamated.hpp>
#include "../src/config_response.hpp"
#include "../src/rules.hpp"

using namespace eppoclient;
using namespace eppoclient::internal;

// GT (Greater Than) Tests

TEST_CASE("FourPartVersion: Version 2.0.0.0 > 1.5.0.0", "[four-part-version]") {
    Condition condition;
    condition.op = Operator::GT;
    condition.attribute = "app_version";
    condition.value = "1.5.0.0";
    condition.precompute();

    Attributes attributes;
    attributes["app_version"] = std::string("2.0.0.0");

    bool result = conditionMatches(condition, attributes, nullptr);
    CHECK(result == true);
}

TEST_CASE("FourPartVersion: Version 1.5.0.0 NOT > 2.0.0.0", "[four-part-version]") {
    Condition condition;
    condition.op = Operator::GT;
    condition.attribute = "app_version";
    condition.value = "2.0.0.0";
    condition.precompute();

    Attributes attributes;
    attributes["app_version"] = std::string("1.5.0.0");

    bool result = conditionMatches(condition, attributes, nullptr);
    CHECK(result == false);
}

TEST_CASE("FourPartVersion: Version 1.5.0.1 > 1.5.0.0", "[four-part-version]") {
    Condition condition;
    condition.op = Operator::GT;
    condition.attribute = "app_version";
    condition.value = "1.5.0.0";
    condition.precompute();

    Attributes attributes;
    attributes["app_version"] = std::string("1.5.0.1");

    bool result = conditionMatches(condition, attributes, nullptr);
    CHECK(result == true);
}

// LT (Less Than) Tests

TEST_CASE("FourPartVersion: Version 1.2.3.0 < 1.5.0.0", "[four-part-version]") {
    Condition condition;
    condition.op = Operator::LT;
    condition.attribute = "app_version";
    condition.value = "1.5.0.0";
    condition.precompute();

    Attributes attributes;
    attributes["app_version"] = std::string("1.2.3.0");

    bool result = conditionMatches(condition, attributes, nullptr);
    CHECK(result == true);
}

TEST_CASE("FourPartVersion: Version 2.0.0.0 NOT < 1.5.0.0", "[four-part-version]") {
    Condition condition;
    condition.op = Operator::LT;
    condition.attribute = "app_version";
    condition.value = "1.5.0.0";
    condition.precompute();

    Attributes attributes;
    attributes["app_version"] = std::string("2.0.0.0");

    bool result = conditionMatches(condition, attributes, nullptr);
    CHECK(result == false);
}

TEST_CASE("FourPartVersion: Version 1.5.0.0 < 1.5.0.1", "[four-part-version]") {
    Condition condition;
    condition.op = Operator::LT;
    condition.attribute = "app_version";
    condition.value = "1.5.0.1";
    condition.precompute();

    Attributes attributes;
    attributes["app_version"] = std::string("1.5.0.0");

    bool result = conditionMatches(condition, attributes, nullptr);
    CHECK(result == true);
}

// GTE (Greater Than or Equal) Tests

TEST_CASE("FourPartVersion: Version 1.5.0.0 >= 1.5.0.0 (equal)", "[four-part-version]") {
    Condition condition;
    condition.op = Operator::GTE;
    condition.attribute = "app_version";
    condition.value = "1.5.0.0";
    condition.precompute();

    Attributes attributes;
    attributes["app_version"] = std::string("1.5.0.0");

    bool result = conditionMatches(condition, attributes, nullptr);
    CHECK(result == true);
}

TEST_CASE("FourPartVersion: Version 1.5.0.1 >= 1.5.0.0 (greater)", "[four-part-version]") {
    Condition condition;
    condition.op = Operator::GTE;
    condition.attribute = "app_version";
    condition.value = "1.5.0.0";
    condition.precompute();

    Attributes attributes;
    attributes["app_version"] = std::string("1.5.0.1");

    bool result = conditionMatches(condition, attributes, nullptr);
    CHECK(result == true);
}

TEST_CASE("FourPartVersion: Version 1.4.0.0 NOT >= 1.5.0.0", "[four-part-version]") {
    Condition condition;
    condition.op = Operator::GTE;
    condition.attribute = "app_version";
    condition.value = "1.5.0.0";
    condition.precompute();

    Attributes attributes;
    attributes["app_version"] = std::string("1.4.0.0");

    bool result = conditionMatches(condition, attributes, nullptr);
    CHECK(result == false);
}

// LTE (Less Than or Equal) Tests

TEST_CASE("FourPartVersion: Version 1.4.9.9 <= 1.5.0.0 (less)", "[four-part-version]") {
    Condition condition;
    condition.op = Operator::LTE;
    condition.attribute = "app_version";
    condition.value = "1.5.0.0";
    condition.precompute();

    Attributes attributes;
    attributes["app_version"] = std::string("1.4.9.9");

    bool result = conditionMatches(condition, attributes, nullptr);
    CHECK(result == true);
}

TEST_CASE("FourPartVersion: Version 1.5.0.0 <= 1.5.0.0 (equal)", "[four-part-version]") {
    Condition condition;
    condition.op = Operator::LTE;
    condition.attribute = "app_version";
    condition.value = "1.5.0.0";
    condition.precompute();

    Attributes attributes;
    attributes["app_version"] = std::string("1.5.0.0");

    bool result = conditionMatches(condition, attributes, nullptr);
    CHECK(result == true);
}

TEST_CASE("FourPartVersion: Version 2.0.0.0 NOT <= 1.5.0.0", "[four-part-version]") {
    Condition condition;
    condition.op = Operator::LTE;
    condition.attribute = "app_version";
    condition.value = "1.5.0.0";
    condition.precompute();

    Attributes attributes;
    attributes["app_version"] = std::string("2.0.0.0");

    bool result = conditionMatches(condition, attributes, nullptr);
    CHECK(result == false);
}

// Three-Part Version Tests

TEST_CASE("FourPartVersion: Three-part version 1.2.3 equals 1.2.3.0", "[four-part-version]") {
    Condition condition;
    condition.op = Operator::GTE;
    condition.attribute = "app_version";
    condition.value = "1.2.3.0";
    condition.precompute();

    Attributes attributes;
    attributes["app_version"] = std::string("1.2.3");

    bool resultGTE = conditionMatches(condition, attributes, nullptr);

    condition.op = Operator::LTE;
    condition.precompute();
    bool resultLTE = conditionMatches(condition, attributes, nullptr);

    CHECK(resultGTE == true);
    CHECK(resultLTE == true);
}

TEST_CASE("FourPartVersion: Version 1.2.3.1 > 1.2.3 (three-part)", "[four-part-version]") {
    Condition condition;
    condition.op = Operator::GT;
    condition.attribute = "app_version";
    condition.value = "1.2.3";
    condition.precompute();

    Attributes attributes;
    attributes["app_version"] = std::string("1.2.3.1");

    bool result = conditionMatches(condition, attributes, nullptr);
    CHECK(result == true);
}

// Lexicographical Comparison Tests

TEST_CASE("FourPartVersion: Version 2.0.0.0 > 1.9.9.9 (first part differs)", "[four-part-version]") {
    Condition condition;
    condition.op = Operator::GT;
    condition.attribute = "app_version";
    condition.value = "1.9.9.9";
    condition.precompute();

    Attributes attributes;
    attributes["app_version"] = std::string("2.0.0.0");

    bool result = conditionMatches(condition, attributes, nullptr);
    CHECK(result == true);
}

TEST_CASE("FourPartVersion: Version 1.5.0.0 > 1.4.9.9 (second part differs)", "[four-part-version]") {
    Condition condition;
    condition.op = Operator::GT;
    condition.attribute = "app_version";
    condition.value = "1.4.9.9";
    condition.precompute();

    Attributes attributes;
    attributes["app_version"] = std::string("1.5.0.0");

    bool result = conditionMatches(condition, attributes, nullptr);
    CHECK(result == true);
}

TEST_CASE("FourPartVersion: Version 1.1.5.0 > 1.1.4.9 (third part differs)", "[four-part-version]") {
    Condition condition;
    condition.op = Operator::GT;
    condition.attribute = "app_version";
    condition.value = "1.1.4.9";
    condition.precompute();

    Attributes attributes;
    attributes["app_version"] = std::string("1.1.5.0");

    bool result = conditionMatches(condition, attributes, nullptr);
    CHECK(result == true);
}

TEST_CASE("FourPartVersion: Version 1.1.1.5 > 1.1.1.4 (fourth part differs)", "[four-part-version]") {
    Condition condition;
    condition.op = Operator::GT;
    condition.attribute = "app_version";
    condition.value = "1.1.1.4";
    condition.precompute();

    Attributes attributes;
    attributes["app_version"] = std::string("1.1.1.5");

    bool result = conditionMatches(condition, attributes, nullptr);
    CHECK(result == true);
}

TEST_CASE("Numeric fallback when not a valid four-part version", "[four-part-version]") {
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

TEST_CASE("FourPartVersion: Invalid version with 4 dots (5 parts)", "[four-part-version]") {
    Condition condition;
    condition.op = Operator::GT;
    condition.attribute = "app_version";
    condition.value = "1.2.3.4";
    condition.precompute();

    Attributes attributes;
    attributes["app_version"] = std::string("1.2.3.4.5");

    bool result = conditionMatches(condition, attributes, nullptr);
    CHECK(result == false);
}

TEST_CASE("FourPartVersion: Invalid version with trailing dot", "[four-part-version]") {
    Condition condition;
    condition.op = Operator::GT;
    condition.attribute = "app_version";
    condition.value = "1.2.3.4";
    condition.precompute();

    Attributes attributes;
    attributes["app_version"] = std::string("1.2.3.");

    bool result = conditionMatches(condition, attributes, nullptr);
    CHECK(result == false);
}

TEST_CASE("FourPartVersion: Invalid version with leading dot", "[four-part-version]") {
    Condition condition;
    condition.op = Operator::GT;
    condition.attribute = "app_version";
    condition.value = "1.2.3.4";
    condition.precompute();

    Attributes attributes;
    attributes["app_version"] = std::string(".1.2.3");

    bool result = conditionMatches(condition, attributes, nullptr);
    CHECK(result == false);
}

TEST_CASE("FourPartVersion: Invalid version with double dots", "[four-part-version]") {
    Condition condition;
    condition.op = Operator::GT;
    condition.attribute = "app_version";
    condition.value = "1.2.3.4";
    condition.precompute();

    Attributes attributes;
    attributes["app_version"] = std::string("1.2..3");

    bool result = conditionMatches(condition, attributes, nullptr);
    CHECK(result == false);
}

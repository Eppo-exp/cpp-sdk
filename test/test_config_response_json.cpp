#include <catch_amalgamated.hpp>
#include "../src/config_response.hpp"
#include "../src/configuration.hpp"
#include <nlohmann/json.hpp>
#include <fstream>

using namespace eppoclient;
using json = nlohmann::json;

TEST_CASE("ConfigResponse serialization - empty config", "[config_response][json]") {
    ConfigResponse response;

    // Serialize to JSON
    json j = response;

    // Check structure
    REQUIRE(j.contains("flags"));
    REQUIRE(j["flags"].is_object());
    REQUIRE(j["flags"].empty());
}

TEST_CASE("ConfigResponse serialization - single simple flag", "[config_response][json]") {
    ConfigResponse response;

    // Create a simple flag
    FlagConfiguration flag;
    flag.key = "test-flag";
    flag.enabled = true;
    flag.variationType = VariationType::STRING;
    flag.totalShards = 10000;

    // Add a variation
    Variation var;
    var.key = "control";
    var.value = "control-value";
    flag.variations["control"] = var;

    response.flags["test-flag"] = flag;

    // Serialize to JSON
    json j = response;

    // Verify structure
    REQUIRE(j.contains("flags"));
    REQUIRE(j["flags"].contains("test-flag"));
    REQUIRE(j["flags"]["test-flag"]["key"] == "test-flag");
    REQUIRE(j["flags"]["test-flag"]["enabled"] == true);
    REQUIRE(j["flags"]["test-flag"]["variationType"] == "STRING");
    REQUIRE(j["flags"]["test-flag"]["totalShards"] == 10000);
    REQUIRE(j["flags"]["test-flag"]["variations"].contains("control"));
}

TEST_CASE("ConfigResponse serialization - multiple variation types", "[config_response][json]") {
    ConfigResponse response;

    // String flag
    FlagConfiguration stringFlag;
    stringFlag.key = "string-flag";
    stringFlag.enabled = true;
    stringFlag.variationType = VariationType::STRING;
    stringFlag.totalShards = 10000;
    response.flags["string-flag"] = stringFlag;

    // Integer flag
    FlagConfiguration intFlag;
    intFlag.key = "int-flag";
    intFlag.enabled = true;
    intFlag.variationType = VariationType::INTEGER;
    intFlag.totalShards = 10000;
    response.flags["int-flag"] = intFlag;

    // Numeric flag
    FlagConfiguration numericFlag;
    numericFlag.key = "numeric-flag";
    numericFlag.enabled = true;
    numericFlag.variationType = VariationType::NUMERIC;
    numericFlag.totalShards = 10000;
    response.flags["numeric-flag"] = numericFlag;

    // Boolean flag
    FlagConfiguration boolFlag;
    boolFlag.key = "bool-flag";
    boolFlag.enabled = true;
    boolFlag.variationType = VariationType::BOOLEAN;
    boolFlag.totalShards = 10000;
    response.flags["bool-flag"] = boolFlag;

    // JSON flag
    FlagConfiguration jsonFlag;
    jsonFlag.key = "json-flag";
    jsonFlag.enabled = true;
    jsonFlag.variationType = VariationType::JSON;
    jsonFlag.totalShards = 10000;
    response.flags["json-flag"] = jsonFlag;

    // Serialize to JSON
    json j = response;

    // Verify all variation types are serialized correctly
    REQUIRE(j["flags"]["string-flag"]["variationType"] == "STRING");
    REQUIRE(j["flags"]["int-flag"]["variationType"] == "INTEGER");
    REQUIRE(j["flags"]["numeric-flag"]["variationType"] == "NUMERIC");
    REQUIRE(j["flags"]["bool-flag"]["variationType"] == "BOOLEAN");
    REQUIRE(j["flags"]["json-flag"]["variationType"] == "JSON");
}

TEST_CASE("ConfigResponse serialization - flag with allocations", "[config_response][json]") {
    ConfigResponse response;

    FlagConfiguration flag;
    flag.key = "allocated-flag";
    flag.enabled = true;
    flag.variationType = VariationType::STRING;
    flag.totalShards = 10000;

    // Add allocation with rule and condition
    Allocation alloc;
    alloc.key = "allocation-1";

    // Add a rule with a condition
    Rule rule;
    Condition cond;
    cond.op = Operator::GT;
    cond.attribute = "age";
    cond.value = 18;
    rule.conditions.push_back(cond);
    alloc.rules.push_back(rule);

    // Add a split
    Split split;
    split.variationKey = "treatment";

    // Add a shard
    Shard shard;
    shard.salt = "test-salt";
    ShardRange range;
    range.start = 0;
    range.end = 5000;
    shard.ranges.push_back(range);
    split.shards.push_back(shard);

    alloc.splits.push_back(split);
    flag.allocations.push_back(alloc);

    response.flags["allocated-flag"] = flag;

    // Serialize to JSON
    json j = response;

    // Verify allocation structure
    REQUIRE(j["flags"]["allocated-flag"]["allocations"].is_array());
    REQUIRE(j["flags"]["allocated-flag"]["allocations"].size() == 1);
    REQUIRE(j["flags"]["allocated-flag"]["allocations"][0]["key"] == "allocation-1");
    REQUIRE(j["flags"]["allocated-flag"]["allocations"][0]["rules"].size() == 1);
    REQUIRE(j["flags"]["allocated-flag"]["allocations"][0]["rules"][0]["conditions"].size() == 1);
    REQUIRE(j["flags"]["allocated-flag"]["allocations"][0]["rules"][0]["conditions"][0]["operator"] == "GT");
    REQUIRE(j["flags"]["allocated-flag"]["allocations"][0]["rules"][0]["conditions"][0]["attribute"] == "age");
}

TEST_CASE("ConfigResponse deserialization - empty config", "[config_response][json]") {
    json j = json::object();
    j["flags"] = json::object();

    ConfigResponse response = j;

    REQUIRE(response.flags.empty());
}

TEST_CASE("ConfigResponse deserialization - single simple flag", "[config_response][json]") {
    json j = R"({
        "flags": {
            "test-flag": {
                "key": "test-flag",
                "enabled": true,
                "variationType": "STRING",
                "variations": {
                    "control": {
                        "key": "control",
                        "value": "control-value"
                    }
                },
                "allocations": [],
                "totalShards": 10000
            }
        }
    })"_json;

    ConfigResponse response = j;

    REQUIRE(response.flags.size() == 1);
    REQUIRE(response.flags.count("test-flag") == 1);
    REQUIRE(response.flags["test-flag"].key == "test-flag");
    REQUIRE(response.flags["test-flag"].enabled == true);
    REQUIRE(response.flags["test-flag"].variationType == VariationType::STRING);
    REQUIRE(response.flags["test-flag"].totalShards == 10000);
    REQUIRE(response.flags["test-flag"].variations.size() == 1);
    REQUIRE(response.flags["test-flag"].variations["control"].key == "control");
}

TEST_CASE("ConfigResponse deserialization - all variation types", "[config_response][json]") {
    json j = R"({
        "flags": {
            "string-flag": {
                "key": "string-flag",
                "enabled": true,
                "variationType": "STRING",
                "variations": {},
                "allocations": [],
                "totalShards": 10000
            },
            "integer-flag": {
                "key": "integer-flag",
                "enabled": true,
                "variationType": "INTEGER",
                "variations": {},
                "allocations": [],
                "totalShards": 10000
            },
            "numeric-flag": {
                "key": "numeric-flag",
                "enabled": true,
                "variationType": "NUMERIC",
                "variations": {},
                "allocations": [],
                "totalShards": 10000
            },
            "boolean-flag": {
                "key": "boolean-flag",
                "enabled": true,
                "variationType": "BOOLEAN",
                "variations": {},
                "allocations": [],
                "totalShards": 10000
            },
            "json-flag": {
                "key": "json-flag",
                "enabled": true,
                "variationType": "JSON",
                "variations": {},
                "allocations": [],
                "totalShards": 10000
            }
        }
    })"_json;

    ConfigResponse response = j;

    REQUIRE(response.flags.size() == 5);
    REQUIRE(response.flags["string-flag"].variationType == VariationType::STRING);
    REQUIRE(response.flags["integer-flag"].variationType == VariationType::INTEGER);
    REQUIRE(response.flags["numeric-flag"].variationType == VariationType::NUMERIC);
    REQUIRE(response.flags["boolean-flag"].variationType == VariationType::BOOLEAN);
    REQUIRE(response.flags["json-flag"].variationType == VariationType::JSON);
}

TEST_CASE("ConfigResponse deserialization - flag with allocations", "[config_response][json]") {
    json j = R"({
        "flags": {
            "allocated-flag": {
                "key": "allocated-flag",
                "enabled": true,
                "variationType": "STRING",
                "variations": {},
                "allocations": [
                    {
                        "key": "allocation-1",
                        "rules": [
                            {
                                "conditions": [
                                    {
                                        "operator": "GT",
                                        "attribute": "age",
                                        "value": 18
                                    }
                                ]
                            }
                        ],
                        "splits": [
                            {
                                "variationKey": "treatment",
                                "shards": [
                                    {
                                        "salt": "test-salt",
                                        "ranges": [
                                            {
                                                "start": 0,
                                                "end": 5000
                                            }
                                        ]
                                    }
                                ],
                                "extraLogging": {}
                            }
                        ]
                    }
                ],
                "totalShards": 10000
            }
        }
    })"_json;

    ConfigResponse response = j;

    REQUIRE(response.flags.size() == 1);
    REQUIRE(response.flags["allocated-flag"].allocations.size() == 1);
    REQUIRE(response.flags["allocated-flag"].allocations[0].key == "allocation-1");
    REQUIRE(response.flags["allocated-flag"].allocations[0].rules.size() == 1);
    REQUIRE(response.flags["allocated-flag"].allocations[0].rules[0].conditions.size() == 1);
    REQUIRE(response.flags["allocated-flag"].allocations[0].rules[0].conditions[0].op == Operator::GT);
    REQUIRE(response.flags["allocated-flag"].allocations[0].rules[0].conditions[0].attribute == "age");
    REQUIRE(response.flags["allocated-flag"].allocations[0].splits.size() == 1);
    REQUIRE(response.flags["allocated-flag"].allocations[0].splits[0].variationKey == "treatment");
}

TEST_CASE("ConfigResponse deserialization - from real UFC file", "[config_response][json]") {
    std::ifstream file("test/data/ufc/flags-v1.json");
    REQUIRE(file.is_open());

    json j;
    file >> j;

    ConfigResponse response = j;

    // Verify some expected flags from the test file
    REQUIRE(response.flags.size() > 0);
    REQUIRE(response.flags.count("empty_flag") == 1);
    REQUIRE(response.flags["empty_flag"].enabled == true);
    REQUIRE(response.flags["empty_flag"].variationType == VariationType::STRING);

    if (response.flags.count("numeric_flag")) {
        REQUIRE(response.flags["numeric_flag"].variationType == VariationType::NUMERIC);
    }
}

TEST_CASE("ConfigResponse round-trip - simple flag", "[config_response][json]") {
    // Create original
    ConfigResponse original;
    FlagConfiguration flag;
    flag.key = "test-flag";
    flag.enabled = true;
    flag.variationType = VariationType::INTEGER;
    flag.totalShards = 10000;

    Variation var1;
    var1.key = "control";
    var1.value = 10;
    flag.variations["control"] = var1;

    Variation var2;
    var2.key = "treatment";
    var2.value = 20;
    flag.variations["treatment"] = var2;

    original.flags["test-flag"] = flag;

    // Serialize
    json j = original;

    // Deserialize
    ConfigResponse deserialized = j;

    // Verify
    REQUIRE(deserialized.flags.size() == 1);
    REQUIRE(deserialized.flags["test-flag"].key == "test-flag");
    REQUIRE(deserialized.flags["test-flag"].enabled == true);
    REQUIRE(deserialized.flags["test-flag"].variationType == VariationType::INTEGER);
    REQUIRE(deserialized.flags["test-flag"].totalShards == 10000);
    REQUIRE(deserialized.flags["test-flag"].variations.size() == 2);
    REQUIRE(deserialized.flags["test-flag"].variations["control"].key == "control");
    REQUIRE(deserialized.flags["test-flag"].variations["treatment"].key == "treatment");
}

TEST_CASE("ConfigResponse round-trip - complex flag with allocations", "[config_response][json]") {
    // Create original with complex structure
    ConfigResponse original;
    FlagConfiguration flag;
    flag.key = "complex-flag";
    flag.enabled = true;
    flag.variationType = VariationType::STRING;
    flag.totalShards = 10000;

    // Add variations
    Variation var1;
    var1.key = "control";
    var1.value = "control-value";
    flag.variations["control"] = var1;

    Variation var2;
    var2.key = "treatment";
    var2.value = "treatment-value";
    flag.variations["treatment"] = var2;

    // Add allocation
    Allocation alloc;
    alloc.key = "allocation-1";

    // Add rule with multiple conditions
    Rule rule;

    Condition cond1;
    cond1.op = Operator::GT;
    cond1.attribute = "age";
    cond1.value = 18;
    rule.conditions.push_back(cond1);

    Condition cond2;
    cond2.op = Operator::MATCHES;
    cond2.attribute = "country";
    cond2.value = "US";
    rule.conditions.push_back(cond2);

    alloc.rules.push_back(rule);

    // Add splits with shards
    Split split;
    split.variationKey = "treatment";

    Shard shard;
    shard.salt = "test-salt";

    ShardRange range1;
    range1.start = 0;
    range1.end = 3000;
    shard.ranges.push_back(range1);

    ShardRange range2;
    range2.start = 5000;
    range2.end = 7000;
    shard.ranges.push_back(range2);

    split.shards.push_back(shard);
    split.extraLogging = json::object();

    alloc.splits.push_back(split);
    flag.allocations.push_back(alloc);

    original.flags["complex-flag"] = flag;

    // Serialize
    json j = original;

    // Deserialize
    ConfigResponse deserialized = j;

    // Verify structure is preserved
    REQUIRE(deserialized.flags.size() == 1);
    REQUIRE(deserialized.flags["complex-flag"].key == "complex-flag");
    REQUIRE(deserialized.flags["complex-flag"].variations.size() == 2);
    REQUIRE(deserialized.flags["complex-flag"].allocations.size() == 1);
    REQUIRE(deserialized.flags["complex-flag"].allocations[0].rules.size() == 1);
    REQUIRE(deserialized.flags["complex-flag"].allocations[0].rules[0].conditions.size() == 2);
    REQUIRE(deserialized.flags["complex-flag"].allocations[0].rules[0].conditions[0].op == Operator::GT);
    REQUIRE(deserialized.flags["complex-flag"].allocations[0].rules[0].conditions[1].op == Operator::MATCHES);
    REQUIRE(deserialized.flags["complex-flag"].allocations[0].splits.size() == 1);
    REQUIRE(deserialized.flags["complex-flag"].allocations[0].splits[0].shards.size() == 1);
    REQUIRE(deserialized.flags["complex-flag"].allocations[0].splits[0].shards[0].ranges.size() == 2);
}

TEST_CASE("ConfigResponse precompute after deserialization", "[config_response][json]") {
    json j = R"({
        "flags": {
            "test-flag": {
                "key": "test-flag",
                "enabled": true,
                "variationType": "INTEGER",
                "variations": {
                    "control": {
                        "key": "control",
                        "value": 10
                    },
                    "treatment": {
                        "key": "treatment",
                        "value": 20
                    }
                },
                "allocations": [],
                "totalShards": 10000
            }
        }
    })"_json;

    ConfigResponse response = j;

    // Precompute should not crash
    REQUIRE_NOTHROW(response.precompute());

    // After precompute, parsed variations should be available
    REQUIRE(response.flags["test-flag"].parsedVariations.size() == 2);
    REQUIRE(response.flags["test-flag"].parsedVariations.count("control") == 1);
    REQUIRE(response.flags["test-flag"].parsedVariations.count("treatment") == 1);
}

TEST_CASE("ConfigResponse usage example - serialize", "[config_response][json]") {
    // This test demonstrates the usage pattern shown in the user's example
    ConfigResponse ufc;

    FlagConfiguration flag;
    flag.key = "example-flag";
    flag.enabled = true;
    flag.variationType = VariationType::STRING;
    flag.totalShards = 10000;

    Variation var;
    var.key = "control";
    var.value = "control-value";
    flag.variations["control"] = var;

    ufc.flags["example-flag"] = flag;

    // Usage: eppoclient::ConfigResponse ufc = loadUFC();
    Configuration configuration = Configuration(ufc);

    // Usage: nlohmann::json ufcJson = ufc;
    nlohmann::json ufcJson = ufc;

    // Usage: std::cout << ufcJson.dump(2) << std::endl;
    std::string jsonString = ufcJson.dump(2);

    // Verify it's valid JSON
    REQUIRE(!jsonString.empty());
    REQUIRE(jsonString.find("example-flag") != std::string::npos);
    REQUIRE(jsonString.find("control") != std::string::npos);
}

TEST_CASE("ConfigResponse usage example - deserialize from file", "[config_response][json]") {
    // This test demonstrates the usage pattern shown in the user's example
    // Usage: eppoclient::ConfigResponse loadFlagsConfiguration(const std::string& filepath)

    const std::string filepath = "test/data/ufc/flags-v1.json";

    std::ifstream file(filepath);
    REQUIRE(file.is_open());

    nlohmann::json j;
    file >> j;

    ConfigResponse response = j;

    // Verify the response is valid
    REQUIRE(response.flags.size() > 0);

    // Can use it with Configuration
    Configuration configuration = Configuration(response);
    REQUIRE_NOTHROW(configuration.precompute());
}

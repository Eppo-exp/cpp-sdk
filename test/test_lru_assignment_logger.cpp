#include <catch_amalgamated.hpp>
#include "../src/lru_assignment_logger.hpp"
#include <memory>
#include <vector>

using namespace eppoclient;

// Mock logger for testing
class MockAssignmentLogger : public AssignmentLogger {
public:
    std::vector<AssignmentEvent> loggedEvents;
    bool shouldThrow = false;

    void logAssignment(const AssignmentEvent& event) override {
        if (shouldThrow) {
            throw std::runtime_error("test exception");
        }
        loggedEvents.push_back(event);
    }

    size_t callCount() const {
        return loggedEvents.size();
    }

    void clear() {
        loggedEvents.clear();
    }
};

namespace {
// Helper to create a test assignment event
AssignmentEvent createTestEvent(
    const std::string& featureFlag = "testFeatureFlag",
    const std::string& allocation = "testAllocation",
    const std::string& variation = "testVariation",
    const std::string& subject = "testSubject",
    const std::string& experiment = "testExperiment",
    const std::string& timestamp = "testTimestamp") {

    AssignmentEvent event;
    event.featureFlag = featureFlag;
    event.allocation = allocation;
    event.variation = variation;
    event.subject = subject;
    event.experiment = experiment;
    event.timestamp = timestamp;
    event.subjectAttributes["testKey"] = std::string("testValue");
    return event;
}
} // anonymous namespace

TEST_CASE("LruAssignmentLogger - cache assignment", "[lru][assignment-logger]") {
    auto innerLogger = std::make_shared<MockAssignmentLogger>();
    auto logger = NewLruAssignmentLogger(innerLogger, 1000);

    AssignmentEvent event = createTestEvent();

    logger->logAssignment(event);
    logger->logAssignment(event);

    CHECK(innerLogger->callCount() == 1);
}

TEST_CASE("LruAssignmentLogger - timestamp and attributes are not important", "[lru][assignment-logger]") {
    auto innerLogger = std::make_shared<MockAssignmentLogger>();
    auto logger = NewLruAssignmentLogger(innerLogger, 1000);

    AssignmentEvent event1 = createTestEvent(
        "testFeatureFlag", "testAllocation", "testVariation",
        "testSubject", "testExperiment", "t1");
    event1.subjectAttributes["testKey"] = std::string("testValue1");

    AssignmentEvent event2 = createTestEvent(
        "testFeatureFlag", "testAllocation", "testVariation",
        "testSubject", "testExperiment", "t2");
    event2.subjectAttributes["testKey"] = std::string("testValue2");

    logger->logAssignment(event1);
    logger->logAssignment(event2);

    CHECK(innerLogger->callCount() == 1);
}

TEST_CASE("LruAssignmentLogger - exceptions are not cached", "[lru][assignment-logger]") {
    auto innerLogger = std::make_shared<MockAssignmentLogger>();
    innerLogger->shouldThrow = true;
    auto logger = NewLruAssignmentLogger(innerLogger, 1000);

    AssignmentEvent event = createTestEvent();

    CHECK_THROWS_AS(logger->logAssignment(event), std::runtime_error);
    CHECK_THROWS_AS(logger->logAssignment(event), std::runtime_error);

    // Should have tried to log twice since it wasn't cached
    CHECK(innerLogger->callCount() == 0); // No successful logs
}

TEST_CASE("LruAssignmentLogger - change in allocation causes logging", "[lru][assignment-logger]") {
    auto innerLogger = std::make_shared<MockAssignmentLogger>();
    auto logger = NewLruAssignmentLogger(innerLogger, 1000);

    AssignmentEvent event1 = createTestEvent(
        "testFeatureFlag", "testAllocation1", "variation",
        "testSubject", "testExperiment", "testTimestamp");

    AssignmentEvent event2 = createTestEvent(
        "testFeatureFlag", "testAllocation2", "variation",
        "testSubject", "testExperiment", "testTimestamp");

    logger->logAssignment(event1);
    logger->logAssignment(event2);

    CHECK(innerLogger->callCount() == 2);
}

TEST_CASE("LruAssignmentLogger - change in variation causes logging", "[lru][assignment-logger]") {
    auto innerLogger = std::make_shared<MockAssignmentLogger>();
    auto logger = NewLruAssignmentLogger(innerLogger, 1000);

    AssignmentEvent event1 = createTestEvent(
        "testFeatureFlag", "testAllocation", "variation1",
        "testSubject", "testExperiment", "testTimestamp");

    AssignmentEvent event2 = createTestEvent(
        "testFeatureFlag", "testAllocation", "variation2",
        "testSubject", "testExperiment", "testTimestamp");

    logger->logAssignment(event1);
    logger->logAssignment(event2);

    CHECK(innerLogger->callCount() == 2);
}

TEST_CASE("LruAssignmentLogger - allocation oscillation logs all", "[lru][assignment-logger]") {
    auto innerLogger = std::make_shared<MockAssignmentLogger>();
    auto logger = NewLruAssignmentLogger(innerLogger, 1000);

    AssignmentEvent event1 = createTestEvent(
        "testFeatureFlag", "testAllocation1", "variation",
        "testSubject", "testExperiment", "t1");

    AssignmentEvent event2 = createTestEvent(
        "testFeatureFlag", "testAllocation2", "variation",
        "testSubject", "testExperiment", "t2");

    AssignmentEvent event3 = createTestEvent(
        "testFeatureFlag", "testAllocation1", "variation",
        "testSubject", "testExperiment", "t3");

    AssignmentEvent event4 = createTestEvent(
        "testFeatureFlag", "testAllocation2", "variation",
        "testSubject", "testExperiment", "t4");

    logger->logAssignment(event1);
    logger->logAssignment(event2);
    logger->logAssignment(event3);
    logger->logAssignment(event4);

    CHECK(innerLogger->callCount() == 4);
}

TEST_CASE("LruAssignmentLogger - variation oscillation logs all", "[lru][assignment-logger]") {
    auto innerLogger = std::make_shared<MockAssignmentLogger>();
    auto logger = NewLruAssignmentLogger(innerLogger, 1000);

    AssignmentEvent event1 = createTestEvent(
        "testFeatureFlag", "testAllocation", "variation1",
        "testSubject", "testExperiment", "t1");

    AssignmentEvent event2 = createTestEvent(
        "testFeatureFlag", "testAllocation", "variation2",
        "testSubject", "testExperiment", "t2");

    AssignmentEvent event3 = createTestEvent(
        "testFeatureFlag", "testAllocation", "variation1",
        "testSubject", "testExperiment", "t3");

    AssignmentEvent event4 = createTestEvent(
        "testFeatureFlag", "testAllocation", "variation2",
        "testSubject", "testExperiment", "t4");

    logger->logAssignment(event1);
    logger->logAssignment(event2);
    logger->logAssignment(event3);
    logger->logAssignment(event4);

    CHECK(innerLogger->callCount() == 4);
}

TEST_CASE("LruAssignmentLogger - different subjects are logged separately", "[lru][assignment-logger]") {
    auto innerLogger = std::make_shared<MockAssignmentLogger>();
    auto logger = NewLruAssignmentLogger(innerLogger, 1000);

    AssignmentEvent event1 = createTestEvent(
        "testFeatureFlag", "testAllocation", "variation",
        "subject1", "testExperiment", "testTimestamp");

    AssignmentEvent event2 = createTestEvent(
        "testFeatureFlag", "testAllocation", "variation",
        "subject2", "testExperiment", "testTimestamp");

    logger->logAssignment(event1);
    logger->logAssignment(event2);

    // Different subjects should both be logged
    CHECK(innerLogger->callCount() == 2);
}

TEST_CASE("LruAssignmentLogger - different flags are logged separately", "[lru][assignment-logger]") {
    auto innerLogger = std::make_shared<MockAssignmentLogger>();
    auto logger = NewLruAssignmentLogger(innerLogger, 1000);

    AssignmentEvent event1 = createTestEvent(
        "flag1", "testAllocation", "variation",
        "testSubject", "testExperiment", "testTimestamp");

    AssignmentEvent event2 = createTestEvent(
        "flag2", "testAllocation", "variation",
        "testSubject", "testExperiment", "testTimestamp");

    logger->logAssignment(event1);
    logger->logAssignment(event2);

    // Different flags should both be logged
    CHECK(innerLogger->callCount() == 2);
}

TEST_CASE("LruAssignmentLogger - constructor validation", "[lru][assignment-logger]") {
    SECTION("throws on null inner logger") {
        CHECK_THROWS_AS(
            LruAssignmentLogger(nullptr, 1000),
            std::invalid_argument
        );
    }

    SECTION("throws on zero cache size") {
        auto innerLogger = std::make_shared<MockAssignmentLogger>();
        CHECK_THROWS_AS(
            LruAssignmentLogger(innerLogger, 0),
            std::invalid_argument
        );
    }
}

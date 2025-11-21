#include <catch_amalgamated.hpp>
#include "../src/lru_bandit_logger.hpp"
#include <memory>
#include <vector>

using namespace eppoclient;

// Mock logger for testing
class MockBanditLogger : public BanditLogger {
public:
    std::vector<BanditEvent> loggedEvents;
    bool shouldThrow = false;

    void logBanditAction(const BanditEvent& event) override {
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
// Helper to create a test bandit event
BanditEvent createTestEvent(
    const std::string& flagKey = "flag",
    const std::string& banditKey = "bandit",
    const std::string& subject = "subject",
    const std::string& action = "action",
    const std::string& modelVersion = "model-version",
    const std::string& timestamp = "timestamp") {

    BanditEvent event;
    event.flagKey = flagKey;
    event.banditKey = banditKey;
    event.subject = subject;
    event.action = action;
    event.actionProbability = 0.1;
    event.optimalityGap = 0.1;
    event.modelVersion = modelVersion;
    event.timestamp = timestamp;
    return event;
}
} // anonymous namespace

TEST_CASE("LruBanditLogger - cache bandit action", "[lru][bandit-logger]") {
    auto innerLogger = std::make_shared<MockBanditLogger>();
    auto logger = NewLruBanditLogger(innerLogger, 1000);

    BanditEvent event = createTestEvent();

    logger->logBanditAction(event);
    logger->logBanditAction(event);

    CHECK(innerLogger->callCount() == 1);
}

TEST_CASE("LruBanditLogger - flip flop action", "[lru][bandit-logger]") {
    auto innerLogger = std::make_shared<MockBanditLogger>();
    auto logger = NewLruBanditLogger(innerLogger, 1000);

    BanditEvent event = createTestEvent("flag", "bandit", "subject", "action1");

    // First action logged
    logger->logBanditAction(event);
    logger->logBanditAction(event);
    CHECK(innerLogger->callCount() == 1);

    // Change to action2, should log
    event.action = "action2";
    logger->logBanditAction(event);
    CHECK(innerLogger->callCount() == 2);

    // Change back to action1, should log again
    event.action = "action1";
    logger->logBanditAction(event);
    CHECK(innerLogger->callCount() == 3);
}

TEST_CASE("LruBanditLogger - different subjects are logged separately", "[lru][bandit-logger]") {
    auto innerLogger = std::make_shared<MockBanditLogger>();
    auto logger = NewLruBanditLogger(innerLogger, 1000);

    BanditEvent event1 = createTestEvent("flag", "bandit", "subject1", "action");
    BanditEvent event2 = createTestEvent("flag", "bandit", "subject2", "action");

    logger->logBanditAction(event1);
    logger->logBanditAction(event2);

    // Different subjects should both be logged
    CHECK(innerLogger->callCount() == 2);
}

TEST_CASE("LruBanditLogger - different flags are logged separately", "[lru][bandit-logger]") {
    auto innerLogger = std::make_shared<MockBanditLogger>();
    auto logger = NewLruBanditLogger(innerLogger, 1000);

    BanditEvent event1 = createTestEvent("flag1", "bandit", "subject", "action");
    BanditEvent event2 = createTestEvent("flag2", "bandit", "subject", "action");

    logger->logBanditAction(event1);
    logger->logBanditAction(event2);

    // Different flags should both be logged
    CHECK(innerLogger->callCount() == 2);
}

TEST_CASE("LruBanditLogger - different bandit keys cause logging", "[lru][bandit-logger]") {
    auto innerLogger = std::make_shared<MockBanditLogger>();
    auto logger = NewLruBanditLogger(innerLogger, 1000);

    BanditEvent event1 = createTestEvent("flag", "bandit1", "subject", "action");
    BanditEvent event2 = createTestEvent("flag", "bandit2", "subject", "action");

    logger->logBanditAction(event1);
    logger->logBanditAction(event2);

    // Different bandit keys should both be logged
    CHECK(innerLogger->callCount() == 2);
}

TEST_CASE("LruBanditLogger - timestamp and probabilities are not important", "[lru][bandit-logger]") {
    auto innerLogger = std::make_shared<MockBanditLogger>();
    auto logger = NewLruBanditLogger(innerLogger, 1000);

    BanditEvent event1 = createTestEvent("flag", "bandit", "subject", "action", "v1", "t1");
    event1.actionProbability = 0.5;
    event1.optimalityGap = 0.2;

    BanditEvent event2 = createTestEvent("flag", "bandit", "subject", "action", "v1", "t2");
    event2.actionProbability = 0.7;
    event2.optimalityGap = 0.3;

    logger->logBanditAction(event1);
    logger->logBanditAction(event2);

    // Should only log once since key fields (flag, bandit, subject, action) are the same
    CHECK(innerLogger->callCount() == 1);
}

// Note: Tests for "exceptions are not cached" and "constructor validation" removed
// since SDK no longer uses exceptions. Constructors now use assert() for precondition checks:
// - inner logger must not be null
// - cache size must be positive
// If a user-provided logger throws an exception, it will propagate and terminate the program.

#ifndef APPLICATION_LOGGER_H
#define APPLICATION_LOGGER_H

#include <string>

namespace eppoclient {

// Application logger interface for logging
class ApplicationLogger {
public:
    virtual ~ApplicationLogger() = default;
    virtual void debug(const std::string& message) = 0;
    virtual void info(const std::string& message) = 0;
    virtual void warn(const std::string& message) = 0;
    virtual void error(const std::string& message) = 0;
};

// No-op implementation of ApplicationLogger
// Used as default when no logger is provided
class NoOpApplicationLogger : public ApplicationLogger {
public:
    void debug(const std::string&) override {}
    void info(const std::string&) override {}
    void warn(const std::string&) override {}
    void error(const std::string&) override {}
};

} // namespace eppoclient

#endif // APPLICATION_LOGGER_H

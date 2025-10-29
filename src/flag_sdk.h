#ifndef FLAG_SDK_H
#define FLAG_SDK_H

#include <string>

namespace flagsdk {

/**
 * Get a boolean value for the given flag key
 * @param flagKey The key of the flag to retrieve
 * @return The boolean value of the flag
 */
bool getBooleanValue(const std::string& flagKey);

} // namespace flagsdk

#endif // FLAG_SDK_H

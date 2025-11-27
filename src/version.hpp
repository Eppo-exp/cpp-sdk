#ifndef EPPOCLIENT_VERSION_HPP
#define EPPOCLIENT_VERSION_HPP

#define EPPOCLIENT_VERSION_MAJOR 2
#define EPPOCLIENT_VERSION_MINOR 0
#define EPPOCLIENT_VERSION_PATCH 0

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define EPPOCLIENT_VERSION             \
    TOSTRING(EPPOCLIENT_VERSION_MAJOR) \
    "." TOSTRING(EPPOCLIENT_VERSION_MINOR) "." TOSTRING(EPPOCLIENT_VERSION_PATCH)

namespace eppoclient {

// Get version as a string
inline const char* getVersion() {
    return EPPOCLIENT_VERSION;
}

}  // namespace eppoclient

#endif  // EPPOCLIENT_VERSION_HPP

#ifndef EPPOCLIENT_VERSION_HPP
#define EPPOCLIENT_VERSION_HPP

#define EPPO_STRINGIFY_HELPER(x) #x
#define EPPO_STRINGIFY(x) EPPO_STRINGIFY_HELPER(x)

#define EPPOCLIENT_VERSION_MAJOR 2
#define EPPOCLIENT_VERSION_MINOR 0
#define EPPOCLIENT_VERSION_PATCH 0
#define EPPOCLIENT_VERSION                                        \
    (EPPO_STRINGIFY(EPPOCLIENT_VERSION_MAJOR) "." EPPO_STRINGIFY( \
        EPPOCLIENT_VERSION_MINOR) "." EPPO_STRINGIFY(EPPOCLIENT_VERSION_PATCH))

namespace eppoclient {

// Get version as a string
inline const char* getVersion() {
    return EPPOCLIENT_VERSION;
}

}  // namespace eppoclient

#endif  // EPPOCLIENT_VERSION_HPP

#ifndef EPPOCLIENT_VERSION_HPP
#define EPPOCLIENT_VERSION_HPP

#define EPPOCLIENT_VERSION_MAJOR 1
#define EPPOCLIENT_VERSION_MINOR 0
#define EPPOCLIENT_VERSION_PATCH 0
#define EPPOCLIENT_VERSION "1.0.0"

namespace eppoclient {

// Get version as a string
inline const char* getVersion() {
    return EPPOCLIENT_VERSION;
}

}  // namespace eppoclient

#endif  // EPPOCLIENT_VERSION_HPP

cmake_minimum_required(VERSION 3.14)

# Read version from version.hpp
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/src/version.hpp" VERSION_MAJOR_LINE REGEX "EPPOCLIENT_VERSION_MAJOR")
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/src/version.hpp" VERSION_MINOR_LINE REGEX "EPPOCLIENT_VERSION_MINOR")
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/src/version.hpp" VERSION_PATCH_LINE REGEX "EPPOCLIENT_VERSION_PATCH")
string(REGEX MATCH "[0-9]+" VERSION_MAJOR ${VERSION_MAJOR_LINE})
string(REGEX MATCH "[0-9]+" VERSION_MINOR ${VERSION_MINOR_LINE})
string(REGEX MATCH "[0-9]+" VERSION_PATCH ${VERSION_PATCH_LINE})

project(eppoclient
    VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
    DESCRIPTION "Eppo C++ SDK for offline flag evaluation"
    LANGUAGES CXX C
)

# Display build information
message(STATUS "Eppo C++ SDK version: ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Processor: ${CMAKE_SYSTEM_PROCESSOR}")

# Detect and display target architecture
if(CMAKE_OSX_ARCHITECTURES)
    message(STATUS "macOS target architectures: ${CMAKE_OSX_ARCHITECTURES}")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    message(STATUS "Target architecture: x86_64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|ARM64")
    message(STATUS "Target architecture: ARM64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "armv7")
    message(STATUS "Target architecture: ARMv7")
else()
    message(STATUS "Target architecture: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set C standard for third party C files
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Collect source files
file(GLOB EPPOCLIENT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# Third party sources (excluding test framework)
set(THIRD_PARTY_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/md5_wrapper.c"
)

# Create the library
add_library(eppoclient STATIC
    ${EPPOCLIENT_SOURCES}
    ${THIRD_PARTY_SOURCES}
)

# Set include directories
target_include_directories(eppoclient
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party>
        $<INSTALL_INTERFACE:include>
)

# Configure nlohmann/json to not use exceptions
target_compile_definitions(eppoclient PUBLIC JSON_NOEXCEPTION)

# Set library properties
set_target_properties(eppoclient PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/src/client.hpp"
)

# Compiler flags
if(MSVC)
    target_compile_options(eppoclient PRIVATE /W4)
    # Disable exceptions on MSVC (PRIVATE so it doesn't affect library users)
    target_compile_options(eppoclient PRIVATE /EHs-c- /D_HAS_EXCEPTIONS=0)
else()
    target_compile_options(eppoclient PRIVATE -Wall -Wextra)
    # Clang-specific warnings
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(eppoclient PRIVATE -Wexit-time-destructors)
    endif()
    # Disable exceptions on GCC/Clang (PRIVATE so it doesn't affect library users)
    target_compile_options(eppoclient PRIVATE -fno-exceptions)
endif()

# Installation
include(GNUInstallDirs)

install(TARGETS eppoclient
    EXPORT eppoclientTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install headers
install(DIRECTORY src/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/eppoclient
    FILES_MATCHING PATTERN "*.hpp"
)

# Install third party headers (needed for public API)
install(DIRECTORY third_party/nlohmann
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/eppoclient/third_party
    FILES_MATCHING PATTERN "*.hpp"
)

install(FILES
    third_party/md5_wrapper.h
    third_party/picohash.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/eppoclient/third_party
)

# Create and install CMake config files for find_package() support
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/eppoclientConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/eppoclientConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/eppoclientConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/eppoclient
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/eppoclientConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/eppoclientConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/eppoclient
)

install(EXPORT eppoclientTargets
    FILE eppoclientTargets.cmake
    NAMESPACE eppoclient::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/eppoclient
)

# Optional: Build examples
option(EPPOCLIENT_BUILD_EXAMPLES "Build example applications" OFF)

if(EPPOCLIENT_BUILD_EXAMPLES)
    add_executable(bandits examples/bandits.cpp)
    target_link_libraries(bandits PRIVATE eppoclient)

    add_executable(flag_assignments examples/flag_assignments.cpp)
    target_link_libraries(flag_assignments PRIVATE eppoclient)

    add_executable(assignment_details examples/assignment_details.cpp)
    target_link_libraries(assignment_details PRIVATE eppoclient)
endif()

# Optional: Build tests
option(EPPOCLIENT_BUILD_TESTS "Build tests" OFF)

if(EPPOCLIENT_BUILD_TESTS)
    enable_testing()

    file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test/test_*.cpp")

    add_executable(test_runner
        ${TEST_SOURCES}
        third_party/catch_amalgamated.cpp
    )

    target_link_libraries(test_runner PRIVATE eppoclient)

    # Re-enable exceptions for test code (Catch2 requires them)
    if(MSVC)
        target_compile_options(test_runner PRIVATE /EHsc /U_HAS_EXCEPTIONS)
    else()
        target_compile_options(test_runner PRIVATE -fexceptions)
    endif()

    add_test(NAME EppoClientTests COMMAND test_runner)
endif()

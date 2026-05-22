cmake_minimum_required(VERSION 3.16)

# mlink_device component: C implementation of the mlink protocol, utils,
# transport, etc. Provides a plain C API via include/mlink.h and friends.

project(mlink_device C)

option(MLINK_DEVICE_BUILD_TESTS "Build tests for mlink_device component" ON)
option(MLINK_DEVICE_BUILD_EXAMPLES "Build examples for mlink_device component" ON)

# Allow this module to decide whether it is built as a static or shared library.
# This keeps the decision close to the component itself instead of forcing a
# global policy from the top-level project.
option(MLINK_DEVICE_SHARED "Build mlink_device as a shared library" ON)

# Collect all C sources under core/ (excluding tests)
file(GLOB_RECURSE MLINK_DEVICE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/core/*.c"
)

if(MLINK_DEVICE_SHARED)
    add_library(mlink_device SHARED ${MLINK_DEVICE_SOURCES})
else()
    add_library(mlink_device STATIC ${MLINK_DEVICE_SOURCES})
endif()


target_include_directories(mlink_device
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/mcp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/transport
        ${CMAKE_CURRENT_SOURCE_DIR}/include/utils
)

target_compile_features(mlink_device PUBLIC c_std_99)

# Enable GNU extensions to ensure pthread_setname_np is properly declared
target_compile_definitions(mlink_device
    PRIVATE
        _GNU_SOURCE
)

# Link against pthread and m (math) where available
find_package(Threads REQUIRED)
target_link_libraries(mlink_device
    PUBLIC
        Threads::Threads
)

if(UNIX AND NOT APPLE)
    # libm is typically needed on Linux
    target_link_libraries(mlink_device PUBLIC m)
endif()

# Optional: integrate libcjson if available via pkg-config
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(CJSON QUIET libcjson)
    if(CJSON_FOUND)
        target_compile_definitions(mlink_device PUBLIC HAVE_CJSON)
        target_include_directories(mlink_device PUBLIC ${CJSON_INCLUDE_DIRS})
        target_link_libraries(mlink_device PUBLIC ${CJSON_LIBRARIES})
    endif()
endif()

install(
    TARGETS mlink_device
    EXPORT ComponentsTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Only install the public API header. Internal headers under include/mcp,
# include/utils, include/transport are used only when building the library.
install(
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/include/mlink.h
    DESTINATION include
)

if(MLINK_DEVICE_BUILD_EXAMPLES)
    add_executable(mlink_device_test
        example/mlink_device_test.c
    )
    target_link_libraries(mlink_device_test
        PRIVATE
            mlink_device
    )
    install(
        TARGETS mlink_device_test
        RUNTIME DESTINATION bin
    )
endif()

# =========================
# Tests (optional, controlled by MLINK_DEVICE_BUILD_TESTS)
# Implemented in tests/CMakeLists.txt
# =========================
if(MLINK_DEVICE_BUILD_TESTS)
    add_subdirectory(tests)
endif()


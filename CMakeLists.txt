cmake_minimum_required(VERSION 2.8)
set(project_name "SSF")
project(${project_name})

set(SSF_VERSION_MAJOR 2)
set(SSF_VERSION_MINOR 2)
set(SSF_VERSION_FIX 0)
set(SSF_VERSION_BOUNCE 2)
set(SSF_VERSION_TRANSPORT 1)

set(SSF_VERSION "${SSF_VERSION_MAJOR}.${SSF_VERSION_MINOR}.${SSF_VERSION_FIX}")

message("** SSF_VERSION: ${SSF_VERSION}")

message("** BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

# --- Options
option(BUILD_UNIT_TESTS "Build SSF unit tests" OFF)
option(BUILD_FRAMEWORK_UNIT_TESTS "Build SSF framework unit tests" OFF)
option(SSF_SECURITY "Activate TLS layer over TCP (FORCE_TCP_ONLY or STANDARD)" "STANDARD")

# --- Set client/server security
if(SSF_SECURITY STREQUAL "FORCE_TCP_ONLY")
  message("** SSF_SECURITY is FORCE_TCP_ONLY")
  add_definitions(-DTCP_ONLY_LINK)
  set(SSF_VERSION_SECURITY 0)
else()
  message("** SSF_SECURITY is STANDARD")
  add_definitions(-DTLS_OVER_TCP_LINK)
  set(SSF_VERSION_SECURITY 2)
endif()

# --- Include CMake build system scripts
add_subdirectory(cmake)
include(HelpersIdeTarget)

# --- Include platform specific settings
include(Platform)

# --- Boost components
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME ON)
find_package(Boost REQUIRED
             COMPONENTS system
                        serialization
                        program_options
                        date_time
                        filesystem
                        regex
                        thread
                        chrono
                        context
                        coroutine)

# --- OpenSSL components
find_package(OpenSSL REQUIRED)

# --- http-parser components
add_subdirectory(third_party/http-parser)

# --- Test certs
add_subdirectory(certs)

# --- Unit tests
if (BUILD_UNIT_TESTS OR BUILD_FRAMEWORK_UNIT_TESTS)
  include(GTest)

  set(GTEST_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/gtest
      CACHE PATH "Path of gtest library")

  # --- Extract GTest archive in build directory
  gtest_unpack_archive()

  enable_testing()

  # --- Add GTest project
  add_subdirectory(${GTEST_ROOT_DIR} third_party/gtest)
endif (BUILD_UNIT_TESTS OR BUILD_FRAMEWORK_UNIT_TESTS)

# -- Images
add_subdirectory(img)

# --- Add sources
add_subdirectory(src)

cmake_minimum_required(VERSION 2.8)
set(project_name "SSF")
project(${project_name})

set(SSF_VERSION_MAJOR 2)
set(SSF_VERSION_MINOR 2)
set(SSF_VERSION_FIX 0)
set(SSF_VERSION_CIRCUIT 2)
set(SSF_VERSION_TRANSPORT 2)
set(SSF_VERSION_BOUNCE 2)
set(SSF_VERSION_TRANSPORT 1)

set(SSF_VERSION "${SSF_VERSION_MAJOR}.${SSF_VERSION_MINOR}.${SSF_VERSION_FIX}")

message("** SSF_VERSION: ${SSF_VERSION}")

message("** BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

# --- Options
option(BUILD_UNIT_TESTS "Build SSF unit tests" OFF)
option(BUILD_NETWORK_UNIT_TESTS "Build SSF network layers unit tests" OFF)
option(SSF_SECURITY "Activate TLS layer over TCP (FORCE_TCP_ONLY or STANDARD)" "STANDARD")
option(USE_STATIC_LIBS "Link binaries statically against libraries" OFF)

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
set(Boost_USE_STATIC_LIBS ${USE_STATIC_LIBS})
set(Boost_USE_MULTITHREADED ON)
find_package(Boost REQUIRED
             COMPONENTS system
                        program_options
                        date_time
                        filesystem
                        regex
                        chrono
                        thread)

# --- OpenSSL components
set(OPENSSL_USE_STATIC_LIBS ${USE_STATIC_LIBS})
find_package(OpenSSL REQUIRED)
if(OPENSSL_USE_STATIC_LIBS AND MSVC_VERSION)
  include(CheckFunctionExists)
  CHECK_FUNCTION_EXISTS("_snprintf" HAVE_SNPRINTF)
  if(NOT HAVE_SNPRINTF)
    list(APPEND OPENSSL_LIBRARIES legacy_stdio_definitions.lib)
  endif(NOT HAVE_SNPRINTF)
  CHECK_FUNCTION_EXISTS("__iob_func" HAVE_IOB_FUNC)
  if(NOT HAVE_IOB_FUNC)
    list(APPEND OPENSSL_LIBRARIES iob_func)
  endif(NOT HAVE_IOB_FUNC)
endif()

# --- http-parser components
add_subdirectory(third_party/http-parser)
add_subdirectory(third_party/msgpack)

# --- Test certs
add_subdirectory(certs)

# --- Unit tests
if (BUILD_UNIT_TESTS OR BUILD_FRAMEWORK_UNIT_TESTS)
  enable_testing()
  add_subdirectory(third_party/googletest/googletest)
  project_group("Unit Tests" gtest gtest_main)
endif (BUILD_UNIT_TESTS OR BUILD_FRAMEWORK_UNIT_TESTS)

# -- Images
add_subdirectory(img)

# --- Add sources
add_subdirectory(src)

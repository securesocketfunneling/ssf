cmake_minimum_required(VERSION 2.8)
set(project_name "SSF")
project(${project_name})

set(SSF_VERSION_MAJOR 2)
set(SSF_VERSION_MINOR 0)
set(SSF_VERSION_FIX 0)
set(SSF_VERSION_BOUNCE 2)
set(SSF_VERSION_TRANSPORT 1)

set(SSF_VERSION "${SSF_VERSION_MAJOR}.${SSF_VERSION_MINOR}.${SSF_VERSION_FIX}")

message("** SSF_VERSION ${SSF_VERSION}")

# --- Options
option(BUILD_UNIT_TESTS "Build SSF unit tests" ON)
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

# --- Project variables
set(project_BINARY_DIR "${${project_name}_BINARY_DIR}")
set(project_ROOT_DIR "${${project_name}_SOURCE_DIR}")
set(project_SRC_DIR "${project_ROOT_DIR}/src")
set(project_IMG_DIR "${project_ROOT_DIR}/img")
set(project_THIRDPARTY_DIR "${project_ROOT_DIR}/third_party")

set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR})
set(project_EXEC_DIR_NAME "ssf-${SSF_VERSION}")
set(project_EXEC_DIR "${project_BINARY_DIR}/ssf/${project_EXEC_DIR_NAME}")

# --- Include CMake build system scripts
add_subdirectory("${project_THIRDPARTY_DIR}/cmake-build-system")
include(HelpersIdeTarget)

set(BOOST_ROOT
      "${project_THIRDPARTY_DIR}/boost" CACHE PATH "Path of boost library")
message("** BOOST_ROOT: ${BOOST_ROOT}")

set(OPENSSL_ROOT_DIR
      "${project_THIRDPARTY_DIR}/openssl" CACHE PATH "Path of openssl library")
message("** OPENSSL_ROOT_DIR: ${OPENSSL_ROOT_DIR}")

# ---- Third party parameters
set(BOOST_PLATFORM_FLAGS "")
set(BOOST_PLATFORM_COMPONENTS "")
set(OPENSSL_PLATFORM_COMPONENTS "")

# --- Include platform specific CMakeList
set(ICON_RC "")
set(EXEC_FLAG "")

if (WIN32)
  include(./cmake-ms/CMakeLists.txt)
elseif (UNIX)
  include(./cmake-unix/CMakeLists.txt)
endif ()

# --- Define boost version if not set
if(NOT DEP_BOOST_VERSION)
  set(DEP_BOOST_VERSION 1.56.0)
endif()

# --- Setup Boost dependencies
set(Boost_USE_STATIC_LIBS       ON)
set(Boost_USE_MULTITHREADED     ON)
set(Boost_USE_STATIC_RUNTIME    ON)

# --- Boost components
find_package(
  Boost REQUIRED
  FLAGS
    STATIC
    ${BOOST_PLATFORM_FLAGS}
  WITH_COMPONENTS
    system
    serialization
    program_options
    date_time
    filesystem
    regex
    thread
    chrono
    context
    coroutine
)

# --- OpenSSL components
find_package(
  OpenSSL REQUIRED
  FLAGS
    STATIC
    ${OPENSSL_PLATFORM_FLAGS}
)

# --- http-parser components
add_subdirectory(third_party/http-parser)

# --- Setup version.h file
set(SSF_VERSION_BOOST ${Boost_VERSION})
set(SSF_VERSION_OPENSSL ${OpenSSL_VERSION})

configure_file(${project_SRC_DIR}/versions.h.in ${project_BINARY_DIR}/versions.h)
include_directories(${project_BINARY_DIR})

# --- Test certs
set(SSF_CERT_TEST_ROOT_FILES
    "${project_ROOT_DIR}/certs/certificate.crt"
    "${project_ROOT_DIR}/certs/dh4096.pem"
    "${project_ROOT_DIR}/certs/private.key")
set(SSF_CERT_TEST_TRUSTED_FILES
    "${project_ROOT_DIR}/certs/trusted/ca.crt"
)

# --- Unit tests
if (BUILD_UNIT_TESTS OR BUILD_FRAMEWORK_UNIT_TESTS)
  include(GTest)

  set(GTEST_ROOT_DIR
        "${project_THIRDPARTY_DIR}/gtest" CACHE PATH "Path of gtest library")

  # --- Extract GTest archive in build directory
  gtest_unpack_archive()

  enable_testing()

  # --- Add GTest project
  add_subdirectory(${GTEST_ROOT_DIR})
endif (BUILD_UNIT_TESTS OR BUILD_FRAMEWORK_UNIT_TESTS)

# --- Add sources
add_subdirectory(src)

cmake_minimum_required(VERSION 2.8)
set(project_name "SSF")
project(${project_name})

set(SSF_VERSION_MAJOR 1)
set(SSF_VERSION_MINOR 0)
set(SSF_VERSION_BOUNCE 1)
set(SSF_VERSION_TRANSPORT 1)

# --- Set client/server security
if(SSF_SECURITY STREQUAL "FORCE_TCP_ONLY")
  message("** SSF_SECURITY is FORCE_TCP_ONLY")
  add_definitions(-DTCP_ONLY_LINK)
  set(SSF_VERSION_SECURITY 0)
else()
  message("** SSF_SECURITY is STANDARD")
  add_definitions(-DTLS_OVER_TCP_LINK)
  set(SSF_VERSION_SECURITY 1)
endif()

# --- Include CMake build components and CMake common files
add_subdirectory(./cmake)
include(HelpersIdeTarget)

# --- Project variables
set(project_BINARY_DIR "${${project_name}_BINARY_DIR}")
set(project_ROOT_DIR "${${project_name}_SOURCE_DIR}")
set(project_SRC_DIR "${project_ROOT_DIR}/src")
set(project_IMG_DIR "${project_ROOT_DIR}/img")
set(project_LIB_DIR "${project_SRC_DIR}/lib")
set(project_THIRDPARTY_DIR "${project_ROOT_DIR}/third_party")

# --- Project files declaration
set(VERSIONS_FILES
      "${project_BINARY_DIR}/versions.h")

FILE(GLOB_RECURSE COMMON_BOOST_FIBER_FILES
      "${project_SRC_DIR}/common/boost/fiber/*.hpp"
      "${project_SRC_DIR}/common/boost/fiber/*.cpp")

FILE(GLOB_RECURSE COMMON_COMMAND_LINE_FILES
      "${project_SRC_DIR}/core/command_line/*.h"
      "${project_SRC_DIR}/core/command_line/*.cpp")

FILE(GLOB_RECURSE COMMON_CONFIG_FILES
      "${project_SRC_DIR}/common/config/*.h"
      "${project_SRC_DIR}/common/config/*.cpp")

FILE(GLOB_RECURSE COMMON_ERROR_FILES
      "${project_SRC_DIR}/common/error/*.h"
      "${project_SRC_DIR}/common/error/*.cpp")

FILE(GLOB_RECURSE COMMON_UTILS_FILES
      "${project_SRC_DIR}/common/utils/*.h"
      "${project_SRC_DIR}/common/utils/*.cpp"
      "${project_SRC_DIR}/common/utils/*.ipp")

FILE(GLOB_RECURSE CORE_TRANSPORT_POLICIES_FILES
      "${project_SRC_DIR}/core/transport_virtual_layer_policies/*.h"
      "${project_SRC_DIR}/core/transport_virtual_layer_policies/*.cpp"
      "${project_SRC_DIR}/core/transport_virtual_layer_policies/*.ipp")

FILE(GLOB_RECURSE CORE_NETWORK_VIRTUAL_LAYER_POLICIES_FILES
      "${project_SRC_DIR}/core/network_virtual_layer_policies/*.h"
      "${project_SRC_DIR}/core/network_virtual_layer_policies/*.cpp"
      "${project_SRC_DIR}/core/network_virtual_layer_policies/*.ipp")

FILE(GLOB_RECURSE CORE_FACTORIES_FILES
      "${project_SRC_DIR}/core/factories/*.h"
      "${project_SRC_DIR}/core/factories/*.cpp"
      "${project_SRC_DIR}/core/factories/*.ipp"
      "${project_SRC_DIR}/core/factory_manager/*.h"
      "${project_SRC_DIR}/core/factory_manager/*.cpp"
      "${project_SRC_DIR}/core/factory_manager/*.ipp"
      "${project_SRC_DIR}/core/service_manager/*.h"
      "${project_SRC_DIR}/core/service_manager/*.cpp"
      "${project_SRC_DIR}/core/service_manager/*.ipp")

FILE(GLOB_RECURSE SERVICES_FILES
      "${project_SRC_DIR}/services/*.h"
      "${project_SRC_DIR}/services/*.cpp"
      "${project_SRC_DIR}/services/*.ipp")

set(SSF_SOURCES
      ${VERSIONS_FILES}
      ${COMMON_BOOST_FIBER_FILES}
      ${COMMON_COMMAND_LINE_FILES}
      ${COMMON_CONFIG_FILES}
      ${COMMON_ERROR_FILES}
      ${COMMON_NETWORK_FILES}
      ${COMMON_UTILS_FILES}
      ${CORE_TRANSPORT_POLICIES_FILES}
      ${CORE_NETWORK_VIRTUAL_LAYER_POLICIES_FILES}
      ${CORE_FACTORIES_FILES}
      ${SERVICES_FILES}
)

set(BOOST_ROOT
      "${project_THIRDPARTY_DIR}/boost" CACHE PATH "Path of boost library")
message("** BOOST_ROOT: ${BOOST_ROOT}")

set(OPENSSL_ROOT_DIR
      "${project_THIRDPARTY_DIR}/openssl" CACHE PATH "Path of openssl library")
message("** OPENSSL_ROOT_DIR: ${OPENSSL_ROOT_DIR}")

# --- Include platform specific CMakeList

# ---- Icon parameters
set(ICON_RC "")
set(EXEC_FLAG "")
# ---- Third party parameters
set(BOOST_PLATFORM_FLAGS "")
set(BOOST_PLATFORM_COMPONENTS "")
set(OPENSSL_PLATFORM_COMPONENTS "")

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
    log
    regex
    thread
    chrono
    ${BOOST_PLATFORM_COMPONENTS}
)

# --- OpenSSL components
find_package(
  OpenSSL REQUIRED
  FLAGS
    STATIC
    ${OPENSSL_PLATFORM_FLAGS}
)

# --- Setup version.h file
set(SSF_VERSION_BOOST ${Boost_VERSION})
set(SSF_VERSION_OPENSSL ${OpenSSL_VERSION})

configure_file(${project_SRC_DIR}/versions.h.in ${project_BINARY_DIR}/versions.h)
include_directories(${project_BINARY_DIR})
  
# --- Include source directory
include_directories(${project_SRC_DIR})

set (BUILD_UNIT_TESTS ON)
if (BUILD_UNIT_TESTS)
  include(GTest)
  
  set(GTEST_ROOT_DIR 
        "${project_THIRDPARTY_DIR}/gtest" CACHE PATH "Path of gtest library")

  # --- Extract GTest archive in build directory
  gtest_unpack_archive()

  enable_testing()

  # --- Add GTest project 
  add_subdirectory(${GTEST_ROOT_DIR})
  
  # --- Add src test 
  add_subdirectory("${project_SRC_DIR}/tests")

  # Group all tests in one directory
  project_group(
    "Unit tests"
    load_config_tests
    fiber_asio_tests
    ssf_client_server_tests
    ssf_client_server_cipher_suites_tests
    socks_tests
    remote_socks_tests
    stream_forward_tests
    remote_stream_forward_tests
    udp_forward_tests
    remote_udp_forward_tests
    bouncing_tests
  )
endif (BUILD_UNIT_TESTS)

add_subdirectory("${project_SRC_DIR}/core")

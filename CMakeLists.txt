cmake_minimum_required(VERSION 2.8)
set(project_name "SSF")
project(${project_name})

set(SSF_VERSION_MAJOR 2)
set(SSF_VERSION_MINOR 0)
set(SSF_VERSION_FIX 0)
set(SSF_VERSION_BOUNCE 1)
set(SSF_VERSION_TRANSPORT 1)

set(SSF_VERSION "${SSF_VERSION_MAJOR}.${SSF_VERSION_MINOR}.${SSF_VERSION_FIX}")

message("** SSF_VERSION ${SSF_VERSION}")

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

# --- Project variables
set(project_BINARY_DIR "${${project_name}_BINARY_DIR}")
set(project_ROOT_DIR "${${project_name}_SOURCE_DIR}")
set(project_SRC_DIR "${project_ROOT_DIR}/src")
set(project_IMG_DIR "${project_ROOT_DIR}/img")
set(project_LIB_DIR "${project_SRC_DIR}/lib")
set(project_THIRDPARTY_DIR "${project_ROOT_DIR}/third_party")

set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR})
set(project_EXEC_DIR_NAME "ssf-${SSF_VERSION}")
set(project_EXEC_DIR "${project_BINARY_DIR}/ssf/${project_EXEC_DIR_NAME}")

# --- Include CMake build system scripts
add_subdirectory("${project_THIRDPARTY_DIR}/cmake-build-system")
include(HelpersIdeTarget)

# --- Project files declaration
set(VERSIONS_FILES
      "${project_BINARY_DIR}/versions.h")

FILE(GLOB_RECURSE COMMON_BOOST_FIBER_FILES
      "${project_SRC_DIR}/common/boost/fiber/*.hpp"
      "${project_SRC_DIR}/common/boost/fiber/*.cpp")

FILE(GLOB_RECURSE COMMON_CONFIG_FILES
      "${project_SRC_DIR}/common/config/*.h"
      "${project_SRC_DIR}/common/config/*.cpp")

FILE(GLOB_RECURSE COMMON_ERROR_FILES
      "${project_SRC_DIR}/common/error/*.h"
      "${project_SRC_DIR}/common/error/*.cpp")

FILE(GLOB_RECURSE COMMON_NETWORK_FILES
      "${project_SRC_DIR}/common/network/*.h"
      "${project_SRC_DIR}/common/network/*.cpp")

FILE(GLOB_RECURSE COMMON_UTILS_FILES
      "${project_SRC_DIR}/common/utils/*.h"
      "${project_SRC_DIR}/common/utils/*.cpp"
      "${project_SRC_DIR}/common/utils/*.ipp")

FILE(GLOB CORE_COMMAND_LINE_STANDARD_FILES
      "${project_SRC_DIR}/core/command_line/standard/*.h"
      "${project_SRC_DIR}/core/command_line/standard/*.cpp")

FILE(GLOB CORE_COMMAND_LINE_COPY_FILES
      "${project_SRC_DIR}/core/command_line/copy/*.h"
      "${project_SRC_DIR}/core/command_line/copy/*.cpp")

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

FILE(GLOB BASE_SERVICE_FILES
      "${project_SRC_DIR}/services/*.h"
      "${project_SRC_DIR}/services/*.cpp"
      "${project_SRC_DIR}/services/*.ipp")

FILE(GLOB_RECURSE ADMIN_SERVICE_FILES
      "${project_SRC_DIR}/services/admin/*.h"
      "${project_SRC_DIR}/services/admin/*.cpp"
      "${project_SRC_DIR}/services/admin/*.ipp")

FILE(GLOB_RECURSE DATAGRAMS_TO_FIBERS_SERVICE_FILES
      "${project_SRC_DIR}/services/datagrams_to_fibers/*.h"
      "${project_SRC_DIR}/services/datagrams_to_fibers/*.cpp"
      "${project_SRC_DIR}/services/datagrams_to_fibers/*.ipp")

FILE(GLOB_RECURSE FIBERS_TO_DATAGRAMS_SERVICE_FILES
      "${project_SRC_DIR}/services/fibers_to_datagrams/*.h"
      "${project_SRC_DIR}/services/fibers_to_datagrams/*.cpp"
      "${project_SRC_DIR}/services/fibers_to_datagrams/*.ipp")

FILE(GLOB_RECURSE SOCKETS_TO_FIBERS_SERVICE_FILES
      "${project_SRC_DIR}/services/datagrams_to_fibers/*.h"
      "${project_SRC_DIR}/services/datagrams_to_fibers/*.cpp"
      "${project_SRC_DIR}/services/datagrams_to_fibers/*.ipp")

FILE(GLOB_RECURSE FIBERS_TO_SOCKETS_SERVICE_FILES
      "${project_SRC_DIR}/services/fibers_to_datagrams/*.h"
      "${project_SRC_DIR}/services/fibers_to_datagrams/*.cpp"
      "${project_SRC_DIR}/services/fibers_to_datagrams/*.ipp")

FILE(GLOB_RECURSE SOCKS_SERVICE_FILES
      "${project_SRC_DIR}/services/socks/*.h"
      "${project_SRC_DIR}/services/socks/*.cpp"
      "${project_SRC_DIR}/services/socks/*.ipp")

FILE(GLOB COPY_FILE_SERVICE_FILES
      "${project_SRC_DIR}/services/copy_file/*.h"
      "${project_SRC_DIR}/services/copy_file/*.cpp"
      "${project_SRC_DIR}/services/copy_file/*.ipp"
      "${project_SRC_DIR}/services/copy_file/fiber_to_file/*.h"
      "${project_SRC_DIR}/services/copy_file/fiber_to_file/*.cpp"
      "${project_SRC_DIR}/services/copy_file/fiber_to_file/*.ipp"
      "${project_SRC_DIR}/services/copy_file/file_to_fiber/*.h"
      "${project_SRC_DIR}/services/copy_file/file_to_fiber/*.cpp"
      "${project_SRC_DIR}/services/copy_file/file_to_fiber/*.ipp"
      "${project_SRC_DIR}/services/copy_file/file_enquirer/*.h"
      "${project_SRC_DIR}/services/copy_file/file_enquirer/*.cpp"
      "${project_SRC_DIR}/services/copy_file/file_enquirer/*.ipp"
      "${project_SRC_DIR}/services/copy_file/packet/*.h"
      "${project_SRC_DIR}/services/copy_file/packet/*.cpp"
      "${project_SRC_DIR}/services/copy_file/packet/*.ipp"
      "${project_SRC_DIR}/services/copy_file/filesystem/*.h"
      "${project_SRC_DIR}/services/copy_file/filesystem/*.cpp")

FILE(GLOB_RECURSE USER_SERVICE_FILES
      "${project_SRC_DIR}/services/user_services/*.h"
      "${project_SRC_DIR}/services/user_services/*.cpp"
      "${project_SRC_DIR}/services/user_services/*.ipp")

set(PARSER_FILES
    "${project_SRC_DIR}/core/parser/bounce_parser.h"
    "${project_SRC_DIR}/core/parser/bounce_parser.cpp")

FILE(GLOB_RECURSE CLIENT_FILES
      "${project_SRC_DIR}/core/client/*.h"
      "${project_SRC_DIR}/core/client/*.ipp"
      "${project_SRC_DIR}/core/client/query_factory.cpp")

FILE(GLOB_RECURSE SERVER_FILES
      "${project_SRC_DIR}/core/server/*.h"
      "${project_SRC_DIR}/core/server/*.ipp"
      "${project_SRC_DIR}/core/server/query_factory.cpp")

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

# --- Set macro SSF service files
set(SERVICES_FILES
      ${BASE_SERVICE_FILES}
      ${ADMIN_SERVICE_FILES}
      ${DATAGRAMS_TO_FIBERS_SERVICE_FILES}
      ${FIBERS_TO_DATAGRAMS_SERVICE_FILES}
      ${SOCKETS_TO_FIBERS_SERVICE_FILES}
      ${FIBERS_TO_SOCKETS_SERVICE_FILES}
      ${SOCKS_SERVICE_FILES}
      ${USER_SERVICE_FILES}
      ${COPY_FILE_SERVICE_FILES})

# --- Set macro SSF source files
set(SSF_SOURCES
      ${VERSIONS_FILES}
      ${COMMON_BOOST_FIBER_FILES}
      ${COMMON_CONFIG_FILES}
      ${COMMON_ERROR_FILES}
      ${COMMON_NETWORK_FILES}
      ${COMMON_UTILS_FILES}
      ${CORE_TRANSPORT_POLICIES_FILES}
      ${CORE_NETWORK_VIRTUAL_LAYER_POLICIES_FILES}
      ${CORE_FACTORIES_FILES}
      ${SERVICES_FILES}
      ${PARSER_FILES}
      ${CLIENT_FILES}
      ${SERVER_FILES}
)

add_subdirectory(${project_THIRDPARTY_DIR}/ssf-framework)

# --- Copy test certs in bin directory
set(SSF_CERT_TEST_ROOT_FILES
    "${project_ROOT_DIR}/certs/certificate.crt"
    "${project_ROOT_DIR}/certs/dh4096.pem"
    "${project_ROOT_DIR}/certs/private.key")
set(SSF_CERT_TEST_TRUSTED_FILES
    "${project_ROOT_DIR}/certs/trusted/ca.crt"
)

file(MAKE_DIRECTORY ${project_EXEC_DIR}/certs)
file(MAKE_DIRECTORY ${project_EXEC_DIR}/certs/trusted)

file(COPY ${SSF_CERT_TEST_ROOT_FILES}
  DESTINATION ${project_EXEC_DIR}/certs)
file(COPY ${SSF_CERT_TEST_TRUSTED_FILES}
  DESTINATION ${project_EXEC_DIR}/certs/trusted)

# --- Tests
set (BUILD_UNIT_TESTS ON)
if (BUILD_UNIT_TESTS)
  include(GTest)

  set(GTEST_ROOT_DIR
        "${project_THIRDPARTY_DIR}/gtest" CACHE PATH "Path of gtest library")

  # --- Copy test certs for tests
  set(project_BINARY_TESTS_DIR "${project_BINARY_DIR}/src/tests")
  set(project_BINARY_TESTS_CERT_DIR ${project_BINARY_TESTS_DIR}/certs)

  file(MAKE_DIRECTORY ${project_BINARY_TESTS_CERT_DIR})
  file(MAKE_DIRECTORY ${project_BINARY_TESTS_CERT_DIR}/trusted)

  file(COPY ${SSF_CERT_TEST_ROOT_FILES}
    DESTINATION ${project_BINARY_TESTS_CERT_DIR})
  file(COPY ${SSF_CERT_TEST_TRUSTED_FILES}
    DESTINATION ${project_BINARY_TESTS_CERT_DIR}/trusted)

  # --- Extract GTest archive in build directory
  gtest_unpack_archive()

  enable_testing()

  # --- Add GTest project
  add_subdirectory(${GTEST_ROOT_DIR})

  # --- Add src test
  add_subdirectory("${project_SRC_DIR}/tests")
endif (BUILD_UNIT_TESTS)

add_subdirectory("${project_SRC_DIR}/core")

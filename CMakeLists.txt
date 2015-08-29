cmake_minimum_required(VERSION 2.8)
set(project_name "SSF")
project(${project_name})

set(SSF_VERSION_MAJOR 2)
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
	  
file(GLOB_RECURSE CORE_INIT_PACKETS
      "${project_SRC_DIR}/core/transport_virtual_layer_policies/transport_protocol_policy.h"
      "${project_SRC_DIR}/core/transport_virtual_layer_policies/init_packets/*.h"
      "${project_SRC_DIR}/core/transport_virtual_layer_policies/init_packets/*.cpp"
      "${project_SRC_DIR}/core/transport_virtual_layer_policies/init_packets/*.ipp")

file(GLOB_RECURSE CORE_NETWORK_VIRTUAL_LAYER_POLICY
      "${project_SRC_DIR}/core/network_virtual_layer_policies/bounce_protocol_policy.h"
      "${project_SRC_DIR}/core/network_virtual_layer_policies/link_policies/*.h"
      "${project_SRC_DIR}/core/network_virtual_layer_policies/link_policies/*.cpp"
      "${project_SRC_DIR}/core/network_virtual_layer_policies/link_policies/*.ipp"
      "${project_SRC_DIR}/core/network_virtual_layer_policies/link_authentication_policies/*.h"
      "${project_SRC_DIR}/core/network_virtual_layer_policies/link_authentication_policies/*.cpp"
      "${project_SRC_DIR}/core/network_virtual_layer_policies/link_authentication_policies/*.ipp")
      

file(GLOB_RECURSE CORE_FACTORIES_FILES
      "${project_SRC_DIR}/core/factory_manager/service_factory_manager.h"
      "${project_SRC_DIR}/core/service_manager/service_manager.h"
      "${project_SRC_DIR}/core/factories/*.h"
      "${project_SRC_DIR}/core/factories/*.cpp"
      "${project_SRC_DIR}/core/factories/*.ipp"
      "${project_SRC_DIR}/common/boost/fiber/*.cpp"
      "${project_SRC_DIR}/common/boost/fiber/*.ipp")

file(GLOB_RECURSE CORE_VIRTUAL_NETWORK_FILES
      "${project_SRC_DIR}/core/virtual_network/*.h"
      "${project_SRC_DIR}/core/virtual_network/*.cc"
      "${project_SRC_DIR}/core/virtual_network/*.cpp"
      "${project_SRC_DIR}/core/virtual_network/*.ipp")
      
file(GLOB_RECURSE COMMON_BOOST_HEADER_FILES
      "${project_SRC_DIR}/common/boost/fiber/*.hpp"
      "${project_SRC_DIR}/common/boost/fiber/*.cpp"
      "${project_SRC_DIR}/common/boost/fiber/*.ipp")

set(COMMON_COMMAND_LINE_FILES
      "${project_SRC_DIR}/core/command_line/command_line.h"
      "${project_SRC_DIR}/core/command_line/command_line.cpp")

set(COMMON_CONFIG_FILES
      "${project_SRC_DIR}/common/config/config.h"
      "${project_SRC_DIR}/common/config/config.cpp")

file(GLOB_RECURSE COMMON_IO_FILES
      "${project_SRC_DIR}/common/io/*.h"
      "${project_SRC_DIR}/common/io/*.cpp"
      "${project_SRC_DIR}/common/io/*.ipp")
	  
set(COMMON_ERROR_FILES
      "${project_SRC_DIR}/common/error/error.h"
      "${project_SRC_DIR}/common/error/error.cpp")

file(GLOB_RECURSE COMMON_NETWORK_HEADER_FILES
      "${project_SRC_DIR}/common/network/*.h"
      "${project_SRC_DIR}/common/network/*.cpp"
      "${project_SRC_DIR}/common/network/*.ipp")

file(GLOB_RECURSE SERVICES_ADMIN_SERVICE_FILES
      "${project_SRC_DIR}/services/initialisation.h"
      "${project_SRC_DIR}/services/base_service.h"
      "${project_SRC_DIR}/services/admin/*.h"
      "${project_SRC_DIR}/services/admin/*.cpp"
      "${project_SRC_DIR}/services/admin/*.ipp")
      
file(GLOB_RECURSE SERVICES_SOCKS_FILES
      "${project_SRC_DIR}/services/socks/*.h"
      "${project_SRC_DIR}/services/socks/*.cpp"
      "${project_SRC_DIR}/services/socks/*.ipp")

file(GLOB_RECURSE SERVICES_USER_SERVICES_FILES
      "${project_SRC_DIR}/services/user_services/*.h"
      "${project_SRC_DIR}/services/user_services/*.cpp"
      "${project_SRC_DIR}/services/user_services/*.ipp") 

set(SERVICES_SOCKETS_TO_FIBERS_FILES
      "${project_SRC_DIR}/services/sockets_to_fibers/sockets_to_fibers.h"
      "${project_SRC_DIR}/services/sockets_to_fibers/sockets_to_fibers.ipp")

set(SERVICES_DATAGRAMS_TO_FIBERS_FILES
      "${project_SRC_DIR}/services/datagrams_to_fibers/datagrams_to_fibers.h"
      "${project_SRC_DIR}/services/datagrams_to_fibers/datagrams_to_fibers.ipp")

set(SERVICES_FIBERS_TO_SOCKETS_FILES
      "${project_SRC_DIR}/services/fibers_to_sockets/fibers_to_sockets.h"
      "${project_SRC_DIR}/services/fibers_to_sockets/fibers_to_sockets.ipp")

set(SERVICES_FIBERS_TO_DATAGRAMS_FILES
      "${project_SRC_DIR}/services/fibers_to_datagrams/fibers_to_datagrams.h"
      "${project_SRC_DIR}/services/fibers_to_datagrams/fibers_to_datagrams.ipp")

file(GLOB_RECURSE SYSTEM_FILES
      "${project_SRC_DIR}/core/system/*.h"
      "${project_SRC_DIR}/core/system/*.cpp"
      "${project_SRC_DIR}/core/system/*.ipp")

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
    coroutine
    context
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

  set(CERT_TEST_ROOT_FILES
      "${project_ROOT_DIR}/certs/certificate.crt"
      "${project_ROOT_DIR}/certs/dh4096.pem"
      "${project_ROOT_DIR}/certs/private.key")
  set(CERT_TEST_TRUSTED_FILES
      "${project_ROOT_DIR}/certs/trusted/ca.crt"
  )
      
  file(GLOB_RECURSE SYSTEM_CONFIG_TEST_FILES
      "${project_SRC_DIR}/tests/system/*.json")
      
  file(MAKE_DIRECTORY ${project_BINARY_DIR}/src/tests/system)
  file(MAKE_DIRECTORY ${project_BINARY_DIR}/src/tests/certs)
  file(MAKE_DIRECTORY ${project_BINARY_DIR}/src/tests/certs/trusted)
      
  file(COPY ${SYSTEM_CONFIG_TEST_FILES} DESTINATION ${project_BINARY_DIR}/src/tests/system)
  file(COPY ${CERT_TEST_ROOT_FILES} DESTINATION ${project_BINARY_DIR}/src/tests/certs)
  file(COPY ${CERT_TEST_TRUSTED_FILES} DESTINATION ${project_BINARY_DIR}/src/tests/certs/trusted)
  
  enable_testing()

  # --- Add GTest project 
  add_subdirectory(${GTEST_ROOT_DIR})
  
  # --- Add src test 
  add_subdirectory("${project_SRC_DIR}/tests")
endif (BUILD_UNIT_TESTS)

add_subdirectory("${project_SRC_DIR}/core")

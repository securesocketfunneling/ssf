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
	  
set(CORE_INIT_PACKETS
      "${project_SRC_DIR}/core/transport_virtual_layer_policies/transport_protocol_policy.h"
      "${project_SRC_DIR}/core/transport_virtual_layer_policies/init_packets/ssf_reply.h"
      "${project_SRC_DIR}/core/transport_virtual_layer_policies/init_packets/ssf_reply.cpp"
      "${project_SRC_DIR}/core/transport_virtual_layer_policies/init_packets/ssf_request.h"
      "${project_SRC_DIR}/core/transport_virtual_layer_policies/init_packets/ssf_request.cpp")

set(CORE_NETWORK_VIRTUAL_LAYER_POLICY
      "${project_SRC_DIR}/core/network_virtual_layer_policies/bounce_protocol_policy.h"
      "${project_SRC_DIR}/core/network_virtual_layer_policies/link_policies/ssl_policy.h"
      "${project_SRC_DIR}/core/network_virtual_layer_policies/link_policies/tcp_policy.h"
      "${project_SRC_DIR}/core/network_virtual_layer_policies/link_authentication_policies/null_link_authentication_policy.h")

set(CORE_FACTORIES_FILES
      "${project_SRC_DIR}/core/factories/command_factory.h"
      "${project_SRC_DIR}/core/factory_manager/service_factory_manager.h"
      "${project_SRC_DIR}/core/factories/service_factory.h"
      "${project_SRC_DIR}/core/factories/service_option_factory.h"
      "${project_SRC_DIR}/core/service_manager/service_manager.h")

set(COMMON_BOOST_HEADER_FILES
      "${project_SRC_DIR}/common/boost/fiber/detail/fiber_id.hpp"
      "${project_SRC_DIR}/common/boost/fiber/detail/fiber_buffer.hpp"
      "${project_SRC_DIR}/common/boost/fiber/detail/fiber_header.hpp"
      "${project_SRC_DIR}/common/boost/fiber/stream_fiber.hpp"
      "${project_SRC_DIR}/common/boost/fiber/basic_endpoint.hpp"
      "${project_SRC_DIR}/common/boost/fiber/stream_fiber_service.hpp"
      "${project_SRC_DIR}/common/boost/fiber/detail/basic_fiber_impl.hpp"
      "${project_SRC_DIR}/common/boost/fiber/basic_fiber_demux.hpp"
      "${project_SRC_DIR}/common/boost/fiber/basic_fiber_demux_service.hpp"
      "${project_SRC_DIR}/common/boost/fiber/basic_fiber_demux_service.ipp"
      "${project_SRC_DIR}/common/boost/fiber/detail/basic_fiber_demux_impl.hpp"
      "${project_SRC_DIR}/common/boost/fiber/detail/io_fiber_accept_op.hpp"
      "${project_SRC_DIR}/common/boost/fiber/detail/io_fiber_read_op.hpp"
      "${project_SRC_DIR}/common/boost/fiber/detail/io_fiber_dgr_read_op.hpp"
      "${project_SRC_DIR}/common/boost/fiber/detail/io_ssl_read_op.hpp"
      "${project_SRC_DIR}/common/boost/fiber/detail/io_operation.hpp"
      "${project_SRC_DIR}/common/boost/fiber/fiber_acceptor_service.hpp"
      "${project_SRC_DIR}/common/boost/fiber/datagram_fiber_service.hpp"
      "${project_SRC_DIR}/common/boost/fiber/datagram_fiber.hpp")

set(COMMON_COMMAND_LINE_FILES
      "${project_SRC_DIR}/core/command_line/command_line.h"
      "${project_SRC_DIR}/core/command_line/command_line.cpp")

set(COMMON_CONFIG_FILES
      "${project_SRC_DIR}/common/config/config.h"
      "${project_SRC_DIR}/common/config/config.cpp")

set(COMMON_ERROR_FILES
      "${project_SRC_DIR}/common/error/error.h"
      "${project_SRC_DIR}/common/error/error.cpp")

set(COMMON_NETWORK_HEADER_FILES
      "${project_SRC_DIR}/common/network/base_session.h"
      "${project_SRC_DIR}/common/network/datagram_link.h"
      "${project_SRC_DIR}/common/network/datagram_link_operator.h"
      "${project_SRC_DIR}/common/network/manager.h"
      "${project_SRC_DIR}/common/network/network_policy_traits.h"
      "${project_SRC_DIR}/common/network/session_forwarder.h"
      "${project_SRC_DIR}/common/network/socket_link.h")

set(SERVICES_ADMIN_SERVICE_FILES
      "${project_SRC_DIR}/services/initialisation.h"
      "${project_SRC_DIR}/services/base_service.h"
      "${project_SRC_DIR}/services/admin/admin.h"
      "${project_SRC_DIR}/services/admin/admin_command.h"
      "${project_SRC_DIR}/services/admin/admin.ipp"
      "${project_SRC_DIR}/services/admin/requests/create_service_request.h"
      "${project_SRC_DIR}/services/admin/requests/service_status.h"
      "${project_SRC_DIR}/services/admin/requests/stop_service_request.h")

set(SERVICES_SOCKS_FILES
      "${project_SRC_DIR}/services/socks/socks_server.h"
      "${project_SRC_DIR}/services/socks/socks_version.h"
      "${project_SRC_DIR}/services/socks/v4/reply.cpp"
      "${project_SRC_DIR}/services/socks/v4/reply.h"
      "${project_SRC_DIR}/services/socks/v4/request.cpp"
      "${project_SRC_DIR}/services/socks/v4/request.h"
      "${project_SRC_DIR}/services/socks/v4/session.h"
      "${project_SRC_DIR}/services/socks/v4/session.ipp"
      "${project_SRC_DIR}/services/socks/v5/reply.cpp"
      "${project_SRC_DIR}/services/socks/v5/reply.h"
      "${project_SRC_DIR}/services/socks/v5/request.cpp"
      "${project_SRC_DIR}/services/socks/v5/request.h"
      "${project_SRC_DIR}/services/socks/v5/reply_auth.cpp"
      "${project_SRC_DIR}/services/socks/v5/reply_auth.h"
      "${project_SRC_DIR}/services/socks/v5/request_auth.cpp"
      "${project_SRC_DIR}/services/socks/v5/request_auth.h"
      "${project_SRC_DIR}/services/socks/v5/session.h"
      "${project_SRC_DIR}/services/socks/v5/session.ipp")

set(SERVICES_USER_SERVICES_FILES
      "${project_SRC_DIR}/services/user_services/base_user_service.h"
      "${project_SRC_DIR}/services/user_services/port_forwarding.h"
      "${project_SRC_DIR}/services/user_services/remote_port_forwarding.h"
      "${project_SRC_DIR}/services/user_services/remote_socks.h"
      "${project_SRC_DIR}/services/user_services/socks.h"
      "${project_SRC_DIR}/services/user_services/udp_port_forwarding.h"
      "${project_SRC_DIR}/services/user_services/udp_remote_port_forwarding.h")

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

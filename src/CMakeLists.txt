cmake_minimum_required(VERSION 2.8)

# --- Project files declaration
set(VERSIONS_FILES
      "${project_BINARY_DIR}/versions.h")

FILE(GLOB_RECURSE COMMON_BOOST_FIBER_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/common/boost/fiber/*.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/common/boost/fiber/*.cpp")

FILE(GLOB_RECURSE COMMON_CONFIG_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/common/config/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/common/config/*.cpp")

FILE(GLOB_RECURSE COMMON_ERROR_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/common/error/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/common/error/*.cpp")

FILE(GLOB_RECURSE COMMON_LOG_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/common/log/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/common/log/*.cpp")

FILE(GLOB_RECURSE COMMON_NETWORK_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/common/network/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/common/network/*.cpp")

FILE(GLOB CORE_COMMAND_LINE_STANDARD_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/core/command_line/standard/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/core/command_line/standard/*.cpp")

FILE(GLOB CORE_COMMAND_LINE_COPY_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/core/command_line/copy/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/core/command_line/copy/*.cpp")

SET(CORE_ASYNC_ENGINE_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/core/async_engine.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/core/async_engine.cpp")

SET(CORE_NETWORK_PROTOCOL_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/core/network_protocol.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/core/network_protocol.cpp")

FILE(GLOB_RECURSE CORE_TRANSPORT_POLICIES_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/core/transport_virtual_layer_policies/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/core/transport_virtual_layer_policies/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/core/transport_virtual_layer_policies/*.ipp")

FILE(GLOB_RECURSE CORE_FACTORIES_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/core/factories/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/core/factories/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/core/factories/*.ipp"
  "${CMAKE_CURRENT_SOURCE_DIR}/core/factory_manager/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/core/factory_manager/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/core/factory_manager/*.ipp"
  "${CMAKE_CURRENT_SOURCE_DIR}/core/service_manager/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/core/service_manager/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/core/service_manager/*.ipp")

FILE(GLOB BASE_SERVICE_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/services/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/services/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/services/*.ipp")

FILE(GLOB_RECURSE ADMIN_SERVICE_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/services/admin/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/services/admin/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/services/admin/*.ipp")

FILE(GLOB_RECURSE DATAGRAMS_TO_FIBERS_SERVICE_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/services/datagrams_to_fibers/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/services/datagrams_to_fibers/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/services/datagrams_to_fibers/*.ipp")

FILE(GLOB_RECURSE FIBERS_TO_DATAGRAMS_SERVICE_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/services/fibers_to_datagrams/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/services/fibers_to_datagrams/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/services/fibers_to_datagrams/*.ipp")

FILE(GLOB_RECURSE SOCKETS_TO_FIBERS_SERVICE_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/services/sockets_to_fibers/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/services/sockets_to_fibers/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/services/sockets_to_fibers/*.ipp")

FILE(GLOB_RECURSE FIBERS_TO_SOCKETS_SERVICE_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/services/fibers_to_sockets/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/services/fibers_to_sockets/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/services/fibers_to_sockets/*.ipp")

FILE(GLOB_RECURSE SOCKS_SERVICE_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/services/socks/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/services/socks/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/services/socks/*.ipp")

FILE(GLOB COPY_FILE_SERVICE_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/services/copy_file/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/services/copy_file/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/services/copy_file/fiber_to_file/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/services/copy_file/fiber_to_file/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/services/copy_file/file_to_fiber/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/services/copy_file/file_to_fiber/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/services/copy_file/file_enquirer/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/services/copy_file/file_enquirer/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/services/copy_file/packet/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/services/copy_file/packet/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/services/copy_file/filesystem/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/services/copy_file/filesystem/*.cpp")

FILE(GLOB_RECURSE PROCESS_SERVICE_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/services/process/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/services/process/*.ipp"
  "${CMAKE_CURRENT_SOURCE_DIR}/services/process/*.cpp")

FILE(GLOB_RECURSE USER_SERVICE_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/services/user_services/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/services/user_services/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/services/user_services/*.ipp")
  
set(CIRCUIT_CONFIG_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/core/circuit/config.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/core/circuit/config.cpp")

FILE(GLOB_RECURSE CLIENT_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/core/client/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/core/client/*.ipp")

FILE(GLOB_RECURSE SERVER_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/core/server/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/core/server/*.ipp")

# --- Include windows impl
if (WIN32)
  # --- Filesystem globbing
  FILE(GLOB_RECURSE COPY_FILE_SERVICE_WINDOWS_FILES
      "${CMAKE_CURRENT_SOURCE_DIR}/services/copy_file/filesystem/windows/*.h"
      "${CMAKE_CURRENT_SOURCE_DIR}/services/copy_file/filesystem/windows/*.cpp")

  set(COPY_FILE_SERVICE_FILES
      ${COPY_FILE_SERVICE_FILES}
      ${COPY_FILE_SERVICE_WINDOWS_FILES})
endif (WIN32)

# --- Include linux/unix impl
if (UNIX)
  # --- Filesystem globbing
  FILE(GLOB_RECURSE COPY_FILE_SERVICE_LINUX_FILES
      "services/copy_file/filesystem/linux/*.h"
      "services/copy_file/filesystem/linux/*.cpp")

  set(COPY_FILE_SERVICE_FILES
      ${COPY_FILE_SERVICE_FILES}
      ${COPY_FILE_SERVICE_LINUX_FILES})
endif (UNIX)

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
  ${COPY_FILE_SERVICE_FILES}
  ${PROCESS_SERVICE_FILES}
)

# --- Set macro SSF source files
set(SSF_SOURCES
  ${VERSIONS_FILES}
  ${COMMON_BOOST_FIBER_FILES}
  ${COMMON_CONFIG_FILES}
  ${COMMON_ERROR_FILES}
  ${COMMON_LOG_FILES}
  ${CORE_ASYNC_ENGINE_FILES}
  ${COMMON_NETWORK_FILES}
  ${CORE_NETWORK_PROTOCOL_FILES}
  ${CORE_TRANSPORT_POLICIES_FILES}
  ${CORE_NETWORK_VIRTUAL_LAYER_POLICIES_FILES}
  ${CORE_FACTORIES_FILES}
  ${SERVICES_FILES}
  ${CIRCUIT_CONFIG_FILES}
  ${CLIENT_FILES}
  ${SERVER_FILES}
)

# --- Include source directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(framework)
add_subdirectory(core)

if (BUILD_UNIT_TESTS)
  add_subdirectory(tests)
endif(BUILD_UNIT_TESTS)

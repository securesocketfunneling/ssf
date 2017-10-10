# --- Project files declaration

configure_file(versions.h.in versions.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(SSF_FRAMEWORK_FILES
  ${CMAKE_CURRENT_BINARY_DIR}/versions.h

  # fibers
  common/boost/fiber/basic_endpoint.hpp
  common/boost/fiber/basic_fiber_demux.hpp
  common/boost/fiber/basic_fiber_demux_service.hpp
  common/boost/fiber/basic_fiber_demux_service.ipp
  common/boost/fiber/datagram_fiber.hpp
  common/boost/fiber/datagram_fiber_service.hpp
  common/boost/fiber/detail/basic_fiber_demux_impl.hpp
  common/boost/fiber/detail/basic_fiber_impl.hpp
  common/boost/fiber/detail/fiber_buffer.hpp
  common/boost/fiber/detail/fiber_header.hpp
  common/boost/fiber/detail/fiber_id.hpp
  common/boost/fiber/detail/io_fiber_accept_op.hpp
  common/boost/fiber/detail/io_fiber_dgr_read_op.hpp
  common/boost/fiber/detail/io_fiber_read_op.hpp
  common/boost/fiber/detail/io_operation.hpp
  common/boost/fiber/detail/io_ssl_read_op.hpp
  common/boost/fiber/fiber_acceptor_service.hpp
  common/boost/fiber/stream_fiber.hpp
  common/boost/fiber/stream_fiber_service.hpp

  # config
  common/config/circuit.cpp
  common/config/circuit.h
  common/config/config.cpp
  common/config/config.h
  common/config/proxy.cpp
  common/config/proxy.h
  common/config/services.cpp
  common/config/services.h
  common/config/tls.cpp
  common/config/tls.h

  # errors
  common/error/error.cpp
  common/error/error.h

  # log
  common/log/log.cpp
  common/log/log.h

  # async engine
  core/async_engine.cpp
  core/async_engine.h

  # network
  core/network_protocol.cpp
  core/network_protocol.h

  # ssf protocol
  core/transport_virtual_layer_policies/init_packets/ssf_reply.cpp
  core/transport_virtual_layer_policies/init_packets/ssf_reply.h
  core/transport_virtual_layer_policies/init_packets/ssf_request.cpp
  core/transport_virtual_layer_policies/init_packets/ssf_request.h
  core/transport_virtual_layer_policies/transport_protocol_policy.h

  # commandline
  core/command_line/base.cpp
  core/command_line/base.h
  core/command_line/user_service_option_factory.h

  # commandline/copy
  core/command_line/copy/command_line.cpp
  core/command_line/copy/command_line.h

  # commandline/standard
  core/command_line/standard/command_line.cpp
  core/command_line/standard/command_line.h

  # factories
  core/factories/service_factory.h
  core/factory_manager/service_factory_manager.h
  core/service_manager/service_manager.h

  # microservices
  services/base_service.h
  services/base_service_config.cpp
  services/base_service_config.h

  # microservices/admin
  services/admin/admin.h
  services/admin/admin.ipp
  services/admin/admin_command.h
  services/admin/command_factory.h
  services/admin/requests/create_service_request.h
  services/admin/requests/service_status.h
  services/admin/requests/stop_service_request.h

  # microservices/copy
  services/copy_file/config.cpp
  services/copy_file/config.h
  services/copy_file/fiber_to_file/fiber_to_file.h
  services/copy_file/fiber_to_file/fiber_to_ofstream_session.h
  services/copy_file/filename_buffer.cpp
  services/copy_file/filename_buffer.h
  services/copy_file/filesystem/filesystem.cpp
  services/copy_file/filesystem/filesystem.h
  services/copy_file/file_enquirer/file_enquirer.h
  services/copy_file/file_to_fiber/file_to_fiber.h
  services/copy_file/file_to_fiber/istream_to_fiber_session.h
  services/copy_file/packet/packet.cpp
  services/copy_file/packet/packet.h

  # microservices/datagram
  services/datagram/datagram_link.h
  services/datagram/datagram_link_operator.h

  # microservices/datagram_to_fibers
  services/datagrams_to_fibers/config.cpp
  services/datagrams_to_fibers/config.h
  services/datagrams_to_fibers/datagrams_to_fibers.h
  services/datagrams_to_fibers/datagrams_to_fibers.ipp

  # microservices/fibers_to_datagrams
  services/fibers_to_datagrams/config.cpp
  services/fibers_to_datagrams/config.h
  services/fibers_to_datagrams/fibers_to_datagrams.h
  services/fibers_to_datagrams/fibers_to_datagrams.ipp

  # microservices/fibers_to_sockets
  services/fibers_to_sockets/config.cpp
  services/fibers_to_sockets/config.h
  services/fibers_to_sockets/fibers_to_sockets.h
  services/fibers_to_sockets/fibers_to_sockets.ipp
  services/fibers_to_sockets/session.h

  # microservices/process
  services/process/config.cpp
  services/process/config.h
  services/process/server.h
  services/process/server.ipp

  # microservices/sockets_to_fibers
  services/sockets_to_fibers/config.cpp
  services/sockets_to_fibers/config.h
  services/sockets_to_fibers/session.h
  services/sockets_to_fibers/sockets_to_fibers.h
  services/sockets_to_fibers/sockets_to_fibers.ipp

  # microservices/socks
  services/socks/config.cpp
  services/socks/config.h
  services/socks/socks_server.h
  services/socks/socks_server.ipp
  services/socks/socks_version.h
  services/socks/v4/session.h
  services/socks/v4/session.ipp
  services/socks/v5/session.h
  services/socks/v5/session.ipp

  # services
  services/user_services/base_user_service.h
  services/user_services/copy_file_service.h
  services/user_services/option_parser.cpp
  services/user_services/option_parser.h
  services/user_services/parameters.h
  services/user_services/port_forwarding.h
  services/user_services/process.h
  services/user_services/remote_port_forwarding.h
  services/user_services/remote_process.h
  services/user_services/remote_socks.h
  services/user_services/socks.h
  services/user_services/udp_port_forwarding.h
  services/user_services/udp_remote_port_forwarding.h
  services/user_service_factory.h

  # client
  core/client/client.cpp
  core/client/client.h
  core/client/client_helper.cpp
  core/client/client_helper.h
  core/client/session.h
  core/client/session.ipp
  core/client/status.h

  # server
  core/server/server.h
  core/server/server.ipp
)

# windows impl
if (WIN32)
  list(APPEND SSF_FRAMEWORK_FILES
    # microservices/copy
    services/copy_file/filesystem/windows/win_filesystem.cpp

    # microservices/process
    services/process/windows/session.h
    services/process/windows/session.ipp
  )
endif (WIN32)

# linux/unix impl
if (UNIX)
  list(APPEND SSF_FRAMEWORK_FILES
    # microservices/copy
    services/copy_file/filesystem/linux/unix_filesystem.cpp

    # microservices/process
    services/process/posix/session.h
    services/process/posix/session.ipp
  )
endif (UNIX)

add_subdirectory(network)
add_subdirectory(compat)

add_target(ssf_framework
  TYPE
    LIBRARY ${EXEC_FLAG} STATIC
  FILES
    ${SSF_FRAMEWORK_FILES}
  INCLUDE_DIRECTORIES PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
  LINKS
    boost
    ssl
    ${PLATFORM_SPECIFIC_LIB_DEP}
    ssf_network
)

add_subdirectory(client)
add_subdirectory(server)

if (BUILD_UNIT_TESTS)
  add_subdirectory(tests)
endif(BUILD_UNIT_TESTS)

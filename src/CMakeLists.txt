# --- Project files declaration

configure_file(versions.h.in versions.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include(source_group_by_folder)

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

  # crypto
  common/crypto/hash.h
  common/crypto/md5.h
  common/crypto/md5.cpp
  common/crypto/sha1.h
  common/crypto/sha1.cpp
  common/crypto/sha256.h
  common/crypto/sha256.cpp

  # errors
  common/error/error.cpp
  common/error/error.h

  # filesystem
  common/filesystem/path.h
  common/filesystem/path.cpp
  common/filesystem/filesystem.h
  common/filesystem/filesystem.cpp

  # utils
  common/utils/to_underlying.h

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
  services/service_port.h
  services/service_id.h

  # microservices/admin
  services/admin/admin.h
  services/admin/admin.ipp
  services/admin/admin_command.h
  services/admin/command_factory.h
  services/admin/requests/create_service_request.h
  services/admin/requests/service_status.h
  services/admin/requests/stop_service_request.h

  # microservices/copy
  services/copy/config.cpp
  services/copy/config.h

  services/copy/copy_client.h
  services/copy/copy_server.h
  services/copy/file_acceptor.h
  services/copy/file_sender.h

  services/copy/packet.h
  services/copy/packet.cpp
  services/copy/packet_helper.h

  # copy session
  services/copy/copy_context.h
  services/copy/copy_context.cpp
  services/copy/copy_session.h
  services/copy/error_code.h
  services/copy/error_code.cpp
  services/copy/i_copy_state.h

  services/copy/state/on_abort.h
  services/copy/state/on_abort.cpp

  # copy states receiver
  services/copy/state/receiver/abort_receiver_state.h
  services/copy/state/receiver/receive_file_state.h
  services/copy/state/receiver/send_abort_ack_state.h
  services/copy/state/receiver/send_eof_state.h
  services/copy/state/receiver/send_init_reply_state.h
  services/copy/state/receiver/send_integrity_check_reply_state.h
  services/copy/state/receiver/wait_close_state.h
  services/copy/state/receiver/wait_init_request_state.h
  services/copy/state/receiver/wait_integrity_check_request_state.h

  # copy states sender
  services/copy/state/sender/abort_sender_state.h
  services/copy/state/sender/close_state.h
  services/copy/state/sender/send_file_state.h
  services/copy/state/sender/send_init_request_state.h
  services/copy/state/sender/send_integrity_check_request_state.h
  services/copy/state/sender/wait_abort_ack_state.h
  services/copy/state/sender/wait_eof_state.h
  services/copy/state/sender/wait_init_reply_state.h
  services/copy/state/sender/wait_integrity_check_reply_state.h

  # copy packet
  services/copy/packet/check.h
  services/copy/packet/control.h
  services/copy/packet/data.h
  services/copy/packet/error_code.h
  services/copy/packet/error.h
  services/copy/packet/init.h

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
  services/user_services/copy.h
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
    # microservices/process
    services/process/windows/session.h
    services/process/windows/session.ipp
  )
  list(APPEND PLATFORM_SPECIFIC_LIB_DEP "shlwapi.lib")
endif (WIN32)

# linux/unix impl
if (UNIX)
  list(APPEND SSF_FRAMEWORK_FILES
    # microservices/process
    services/process/posix/session.h
    services/process/posix/session.ipp
  )
endif (UNIX)

add_subdirectory(network)
add_subdirectory(compat)

add_library(ssf_framework STATIC "")
target_sources(ssf_framework PRIVATE ${SSF_FRAMEWORK_FILES})
target_include_directories(ssf_framework PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ssf_framework PUBLIC boost ssl ${PLATFORM_LIBS} ssf_network)
set_property(TARGET ssf_framework PROPERTY FOLDER Libraries)
source_group_by_folder(ssf_framework)

if(DISABLE_TLS)
  target_compile_definitions(ssf_framework PUBLIC TCP_ONLY_LINK)
else()
  target_compile_definitions(ssf_framework PUBLIC TLS_OVER_TCP_LINK)
endif()

add_subdirectory(client)
add_subdirectory(server)

if (BUILD_UNIT_TESTS)
  add_subdirectory(tests)
endif(BUILD_UNIT_TESTS)

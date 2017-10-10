# specific platform lib
set(PLATFORM_LIB "")

set(SSF_NETWORK_FILES
  # error
  ssf/error/error.h
  ssf/error/error.ipp

  # io
  ssf/io/accept_op.h
  ssf/io/buffers.h
  ssf/io/composed_op.h
  ssf/io/connect_op.h
  ssf/io/get_op.h
  ssf/io/handler_helpers.h
  ssf/io/op.h
  ssf/io/push_op.h
  ssf/io/read_op.h
  ssf/io/read_stream_op.h
  ssf/io/write_op.h

  # layer
  ssf/layer/accept_op.h
  ssf/layer/basic_empty_datagram.h
  ssf/layer/basic_empty_stream.h
  ssf/layer/basic_endpoint.h
  ssf/layer/basic_impl.h
  ssf/layer/basic_resolver.h
  ssf/layer/congestion/drop_tail_policy.h
  ssf/layer/connect_op.h
  ssf/layer/io_handler.h
  ssf/layer/parameters.cpp
  ssf/layer/parameters.h
  ssf/layer/protocol_attributes.h
  ssf/layer/receive_from_op.h

  # layer/crypto
  ssf/layer/cryptography/basic_crypto_stream.h
  ssf/layer/cryptography/crypto_stream_op.h
  ssf/layer/cryptography/tls/OpenSSL/helpers.cpp
  ssf/layer/cryptography/tls/OpenSSL/helpers.h
  ssf/layer/cryptography/tls/OpenSSL/impl.h

  # layer/datagram
  ssf/layer/datagram/basic_datagram.h
  ssf/layer/datagram/basic_flags.h
  ssf/layer/datagram/basic_header.h
  ssf/layer/datagram/basic_payload.h
  ssf/layer/datagram/empty_component.h

  # layer/data_link
  ssf/layer/data_link/basic_circuit_acceptor_service.h
  ssf/layer/data_link/basic_circuit_protocol.h
  ssf/layer/data_link/basic_circuit_socket_service.h
  ssf/layer/data_link/circuit_endpoint_context.h
  ssf/layer/data_link/circuit_helpers.cpp
  ssf/layer/data_link/circuit_helpers.h
  ssf/layer/data_link/circuit_op.h
  ssf/layer/data_link/helpers.h
  ssf/layer/data_link/simple_circuit_policy.h

  # layer/interface
  #ssf/layer/interface_layer/basic_interface.h
  #ssf/layer/interface_layer/basic_interface_manager.h
  #ssf/layer/interface_layer/basic_interface_protocol.cc
  #ssf/layer/interface_layer/basic_interface_protocol.h
  #ssf/layer/interface_layer/basic_interface_service.h
  #ssf/layer/interface_layer/generic_interface_socket.h
  #ssf/layer/interface_layer/interface_buffers.h
  #ssf/layer/interface_layer/specific_interface_socket.h

  # layer/multiplexing
  #ssf/layer/multiplexing/basic_demultiplexer.h
  #ssf/layer/multiplexing/basic_multiplexer.h
  #ssf/layer/multiplexing/basic_multiplexer_protocol.h
  #ssf/layer/multiplexing/basic_multiplexer_socket_service.h
  #ssf/layer/multiplexing/demultiplexer_manager.h
  #ssf/layer/multiplexing/multiplexer_manager.h
  #ssf/layer/multiplexing/port_multiplex_id.h
  #ssf/layer/multiplexing/protocol_and_port_multiplex_id.h
  #ssf/layer/multiplexing/protocol_multiplex_id.h

  # layer/network
  #ssf/layer/network/basic_network_protocol.h
  #ssf/layer/network/basic_network_raw_socket_service.h
  #ssf/layer/network/basic_network_socket_service.h
  #ssf/layer/network/network_id.h

  # layer/physical
  ssf/layer/physical/host.cpp
  ssf/layer/physical/host.h
  ssf/layer/physical/tcp.cpp
  ssf/layer/physical/tcp.h
  ssf/layer/physical/tcp_helpers.cpp
  ssf/layer/physical/tcp_helpers.h
  ssf/layer/physical/tlsotcp.h
  ssf/layer/physical/udp.h
  ssf/layer/physical/udp_helpers.cpp
  ssf/layer/physical/udp_helpers.h

  # layer/proxy
  ssf/layer/proxy/auth_strategy.cpp
  ssf/layer/proxy/auth_strategy.h
  ssf/layer/proxy/base64.cpp
  ssf/layer/proxy/base64.h
  ssf/layer/proxy/basic_auth_strategy.cpp
  ssf/layer/proxy/basic_auth_strategy.h
  ssf/layer/proxy/basic_proxy_acceptor_service.h
  ssf/layer/proxy/basic_proxy_protocol.h
  ssf/layer/proxy/basic_proxy_socket_service.h
  ssf/layer/proxy/connect_op.h
  ssf/layer/proxy/digest_auth_strategy.cpp
  ssf/layer/proxy/digest_auth_strategy.h
  ssf/layer/proxy/http_connect_op.h
  ssf/layer/proxy/http_request.cpp
  ssf/layer/proxy/http_request.h
  ssf/layer/proxy/http_response.cpp
  ssf/layer/proxy/http_response.h
  ssf/layer/proxy/http_response_builder.cpp
  ssf/layer/proxy/http_response_builder.h
  ssf/layer/proxy/http_session_initializer.cpp
  ssf/layer/proxy/http_session_initializer.h
  ssf/layer/proxy/negotiate_auth_strategy.cpp
  ssf/layer/proxy/negotiate_auth_strategy.h
  ssf/layer/proxy/ntlm_auth_strategy.cpp
  ssf/layer/proxy/ntlm_auth_strategy.h
  ssf/layer/proxy/platform_auth_impl.h
  ssf/layer/proxy/proxy_endpoint_context.cpp
  ssf/layer/proxy/proxy_endpoint_context.h
  ssf/layer/proxy/proxy_helpers.cpp
  ssf/layer/proxy/proxy_helpers.h
  ssf/layer/proxy/socks4_strategy.cpp
  ssf/layer/proxy/socks4_strategy.h
  ssf/layer/proxy/socks5_strategy.cpp
  ssf/layer/proxy/socks5_strategy.h
  ssf/layer/proxy/socks_connect_op.h
  ssf/layer/proxy/socks_session_initializer.cpp
  ssf/layer/proxy/socks_session_initializer.h
  ssf/layer/proxy/socks_strategy.h

  # layer/queue
  ssf/layer/queue/active_item.h
  ssf/layer/queue/async_queue.h
  ssf/layer/queue/async_queue_service.h
  ssf/layer/queue/commutator.h
  ssf/layer/queue/send_queued_datagram_socket.h
  ssf/layer/queue/tagged_item.h

  # layer/routing
  # ssf/layer/routing/basic_routed_protocol.h
  # ssf/layer/routing/basic_routed_socket_service.h
  # ssf/layer/routing/basic_router.h
  # ssf/layer/routing/basic_router_service.h
  # ssf/layer/routing/basic_routing_selector.h
  # ssf/layer/routing/basic_routing_table.h

  # log
  ssf/log/log.cpp
  ssf/log/log.h

  # network
  ssf/network/base_session.h
  ssf/network/manager.h
  ssf/network/object_io_helpers.h
  ssf/network/session_forwarder.h
  ssf/network/socket_link.h
  ssf/network/socks/socks.h
  ssf/network/socks/v4/reply.cpp
  ssf/network/socks/v4/reply.h
  ssf/network/socks/v4/request.cpp
  ssf/network/socks/v4/request.h
  ssf/network/socks/v5/reply.cpp
  ssf/network/socks/v5/reply.h
  ssf/network/socks/v5/reply_auth.cpp
  ssf/network/socks/v5/reply_auth.h
  ssf/network/socks/v5/request.cpp
  ssf/network/socks/v5/request.h
  ssf/network/socks/v5/request_auth.cpp
  ssf/network/socks/v5/request_auth.h
  ssf/network/socks/v5/types.h

  # router system
  # ssf/system/basic_interfaces_collection.h
  # ssf/system/specific_interfaces_collection.h
  # ssf/system/system_interfaces.cpp
  # ssf/system/system_interfaces.h
  # ssf/system/system_routers.cpp
  # ssf/system/system_routers.h

  # utils
  ssf/utils/cleaner.h
  ssf/utils/enum.h
  ssf/utils/map_helpers.h
)

if (WIN32)
  # windows impl
  list(APPEND PLATFORM_LIB Secur32.lib)
  list(APPEND SSF_NETWORK_FILES
    # layer/proxy
    ssf/layer/proxy/windows/sspi_auth_impl.cpp
    ssf/layer/proxy/windows/sspi_auth_impl.h)
endif (WIN32)
if (UNIX)
  # linux/unix impl
  list(APPEND PLATFORM_LIB dl)
  list(APPEND SSF_NETWORK_FILES
    # layer/proxy
    ssf/layer/proxy/unix/gssapi_auth_impl.cpp
    ssf/layer/proxy/unix/gssapi_auth_impl.h)
endif (UNIX)

add_target(ssf_network
  TYPE
    library ${EXEC_FLAG} STATIC
  FILES
    ${SSF_NETWORK_FILES}
  LINKS
    ${OPENSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
    ${PLATFORM_LIB}
    http_parser
    msgpack_c
  INCLUDE_DIRECTORIES PUBLIC
    ${OPENSSL_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
  PREFIX_SKIP     .*/src
)

if (BUILD_NETWORK_UNIT_TESTS)
  add_subdirectory(tests)
endif(BUILD_NETWORK_UNIT_TESTS)

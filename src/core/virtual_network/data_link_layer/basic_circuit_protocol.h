#ifndef SSF_CORE_VIRTUAL_NETWORK_DATA_LINK_LAYER_CIRCUIT_PROTOCOL_H_
#define SSF_CORE_VIRTUAL_NETWORK_DATA_LINK_LAYER_CIRCUIT_PROTOCOL_H_

#include <cstdint>

#include <map>

#include <boost/asio/io_service.hpp>

#include <boost/system/error_code.hpp>

#include "core/virtual_network/data_link_layer/circuit_helpers.h"
#include "core/virtual_network/data_link_layer/circuit_endpoint_context.h"

#include "core/virtual_network/basic_resolver.h"
#include "core/virtual_network/basic_endpoint.h"

#include "core/virtual_network/protocol_attributes.h"

#include "core/virtual_network/data_link_layer/basic_circuit_acceptor_service.h"
#include "core/virtual_network/data_link_layer/basic_circuit_socket_service.h"

namespace virtual_network {
namespace data_link_layer {

template <class NextLayer, template <class> class CircuitPolicy>
class basic_CircuitProtocol {
 public:
  enum {
    id = 5,
    overhead = 0,
    facilities = virtual_network::facilities::stream,
    mtu = NextLayer::mtu - overhead
  };
  enum { endpoint_stack_size = 1 + NextLayer::endpoint_stack_size };

  typedef CircuitPolicy<basic_CircuitProtocol> circuit_policy;
  typedef NextLayer next_layer_protocol;
  typedef int socket_context;
  typedef int acceptor_context;
  typedef CircuitEndpointContext endpoint_context_type;
  typedef basic_VirtualLink_endpoint<basic_CircuitProtocol>
      endpoint;
  typedef basic_VirtualLink_resolver<basic_CircuitProtocol>
      resolver;
  typedef boost::asio::basic_stream_socket<
      basic_CircuitProtocol,
      basic_CircuitSocket_service<
          basic_CircuitProtocol>> socket;
  typedef boost::asio::basic_socket_acceptor<
      basic_CircuitProtocol,
      basic_CircuitAcceptor_service<
          basic_CircuitProtocol>> acceptor;

 public:
   template <class Container>
   static endpoint
   make_endpoint(boost::asio::io_service &io_service,
                 typename Container::const_iterator parameters_it, uint32_t,
                 boost::system::error_code& ec) {
    auto context = detail::make_circuit_context(io_service, *parameters_it);
    auto next_layer_endpoint = next_layer_protocol::template make_endpoint<Container>(
        io_service, ++parameters_it, id, ec);

    return endpoint(context, next_layer_endpoint);
  }
};

}  // data_link_layer
}  // virtual_network

#endif  // SSF_CORE_VIRTUAL_NETWORK_DATA_LINK_LAYER_CIRCUIT_PROTOCOL_H_

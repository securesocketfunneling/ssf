#ifndef SSF_LAYER_DATA_LINK_CIRCUIT_PROTOCOL_H_
#define SSF_LAYER_DATA_LINK_CIRCUIT_PROTOCOL_H_

#include <cstdint>

#include <map>
#include <thread>

#include <boost/asio/io_service.hpp>
#include <boost/asio/basic_socket_acceptor.hpp>
#include <boost/asio/basic_stream_socket.hpp>

#include <boost/system/error_code.hpp>

#include "ssf/layer/basic_resolver.h"
#include "ssf/layer/basic_endpoint.h"

#include "ssf/layer/data_link/basic_circuit_acceptor_service.h"
#include "ssf/layer/data_link/basic_circuit_socket_service.h"
#include "ssf/layer/data_link/circuit_helpers.h"
#include "ssf/layer/data_link/circuit_endpoint_context.h"

#include "ssf/layer/parameters.h"
#include "ssf/layer/protocol_attributes.h"

namespace ssf {
namespace layer {
namespace data_link {

template <class NextLayer, template <class> class CircuitPolicy>
class basic_CircuitProtocol {
 public:
  enum {
    id = 5,
    overhead = 0,
    facilities = ssf::layer::facilities::stream,
    mtu = NextLayer::mtu - overhead
  };
  enum { endpoint_stack_size = 1 + NextLayer::endpoint_stack_size };

  static const char* NAME;

  typedef CircuitPolicy<basic_CircuitProtocol> circuit_policy;
  typedef NextLayer next_layer_protocol;
  typedef int socket_context;
  typedef int acceptor_context;
  typedef CircuitEndpointContext endpoint_context_type;
  using next_endpoint_type = typename next_layer_protocol::endpoint;

  typedef basic_VirtualLink_endpoint<basic_CircuitProtocol> endpoint;
  typedef basic_VirtualLink_resolver<basic_CircuitProtocol> resolver;
  typedef boost::asio::basic_stream_socket<
      basic_CircuitProtocol, basic_CircuitSocket_service<basic_CircuitProtocol>>
      socket;
  typedef boost::asio::basic_socket_acceptor<
      basic_CircuitProtocol,
      basic_CircuitAcceptor_service<basic_CircuitProtocol>> acceptor;

 private:
  using query = typename resolver::query;

 public:
  static std::string get_name() {
    std::string name(NAME);
    name += "_" + next_layer_protocol::get_name();
    return name;
  }

  static endpoint make_endpoint(boost::asio::io_service& io_service,
                                typename query::const_iterator parameters_it,
                                uint32_t, boost::system::error_code& ec) {
    auto context = detail::make_circuit_context(io_service, *parameters_it);
    auto next_layer_endpoint =
        next_layer_protocol::make_endpoint(io_service, ++parameters_it, id, ec);

    return endpoint(context, next_layer_endpoint);
  }
};

template <class NextLayer, template <class> class CircuitPolicy>
const char* basic_CircuitProtocol<NextLayer, CircuitPolicy>::NAME = "CIRCUIT";

}  // data_link
}  // layer
}  // ssf

#endif  // SSF_LAYER_DATA_LINK_CIRCUIT_PROTOCOL_H_

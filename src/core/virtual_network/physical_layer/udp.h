#ifndef SSF_CORE_VIRTUAL_NETWORK_PHYSICAL_LAYER_UDP_H_
#define SSF_CORE_VIRTUAL_NETWORK_PHYSICAL_LAYER_UDP_H_

#include <cstdint>

#include <memory>

#include <boost/asio/ip/udp.hpp>
#include <boost/asio/io_service.hpp>

#include "core/virtual_network/physical_layer/udp_helpers.h"
#include "core/virtual_network/protocol_attributes.h"

namespace virtual_network {
namespace physical_layer {

class udp {
 public:
  enum {
    id = 11,
    overhead = 0,
    facilities = virtual_network::facilities::datagram,
    mtu = 1500 - overhead
  };
  enum { endpoint_stack_size = 1 };

  typedef int socket_context;
  typedef int acceptor_context;
  typedef boost::asio::ip::udp::socket socket;
  typedef boost::asio::ip::udp::resolver resolver;
  typedef boost::asio::ip::udp::endpoint endpoint;

 public:
  operator boost::asio::ip::udp() { return boost::asio::ip::udp::v4(); }

  template <class Container>
  static endpoint
  make_endpoint(boost::asio::io_service &io_service,
                typename Container::const_iterator parameters_it, uint32_t,
                boost::system::error_code& ec) {
    return endpoint(virtual_network::physical_layer::detail::make_udp_endpoint(
        io_service, *parameters_it, ec));
  }
};

}  // physical_layer
}  // virtual_network

#endif  // SSF_CORE_VIRTUAL_NETWORK_PHYSICAL_LAYER_UDP_H_

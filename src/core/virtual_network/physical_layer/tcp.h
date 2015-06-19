#ifndef SSF_CORE_VIRTUAL_NETWORK_PHYSICAL_LAYER_TCP_H_
#define SSF_CORE_VIRTUAL_NETWORK_PHYSICAL_LAYER_TCP_H_

#include <cstdint>

#include <memory>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>

#include <boost/system/error_code.hpp>

#include "core/virtual_network/physical_layer/tcp_helpers.h"
#include "core/virtual_network/protocol_attributes.h"

namespace virtual_network {
namespace physical_layer {

class tcp {
 public:
  enum {
    id = 1,
    overhead = 0,
    facilities = virtual_network::facilities::stream,
    mtu = 65535 - overhead
  };
  enum { endpoint_stack_size = 1 };

  typedef int socket_context;
  typedef int acceptor_context;
  typedef boost::asio::ip::tcp::socket socket;
  typedef boost::asio::ip::tcp::acceptor acceptor;
  typedef boost::asio::ip::tcp::resolver resolver;
  typedef boost::asio::ip::tcp::endpoint endpoint;

 public:
  operator boost::asio::ip::tcp() { return boost::asio::ip::tcp::v4(); }

  template <class Container>
  static endpoint
  make_endpoint(boost::asio::io_service &io_service,
                typename Container::const_iterator parameters_it, uint32_t,
                boost::system::error_code& ec) {
    return endpoint(virtual_network::physical_layer::detail::make_tcp_endpoint(
        io_service, *parameters_it, ec));
  }
};

}  // physical_layer
}  // virtual_network

#endif  // SSF_CORE_VIRTUAL_NETWORK_PHYSICAL_LAYER_TCP_H_

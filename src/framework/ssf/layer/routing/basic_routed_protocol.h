#ifndef SSF_LAYER_ROUTING_BASIC_ROUTED_PROTOCOL_H_
#define SSF_LAYER_ROUTING_BASIC_ROUTED_PROTOCOL_H_

#include <set>
#include <string>
#include <map>

#include <boost/asio/basic_datagram_socket.hpp>
#include <boost/asio/io_service.hpp>

#include <boost/system/error_code.hpp>

#include "ssf/error/error.h"
#include "ssf/utils/map_helpers.h"

#include "ssf/layer/basic_resolver.h"
#include "ssf/layer/basic_endpoint.h"

#include "ssf/layer/datagram/basic_datagram.h"
#include "ssf/layer/datagram/basic_header.h"
#include "ssf/layer/datagram/basic_payload.h"
#include "ssf/layer/datagram/empty_component.h"

#include "ssf/layer/protocol_attributes.h"

#include "ssf/layer/routing/basic_router.h"
#include "ssf/layer/routing/basic_routed_socket_service.h"

namespace ssf {
namespace layer {
namespace routing {

template <class NextLayer>
class basic_RoutedProtocol {
 public:
  using next_layer_protocol = NextLayer;

  using ReceiveDatagram = typename next_layer_protocol::ReceiveDatagram;
  using SendDatagram = typename next_layer_protocol::SendDatagram;

  using network_address_type =
      typename next_layer_protocol::endpoint_context_type;
  using prefix_type = network_address_type;

  using router_type = basic_Router<next_layer_protocol>;

  enum {
    id = 568,
    overhead = SendDatagram::size,
    facilities = ssf::layer::facilities::datagram,
    mtu = NextLayer::mtu
  };
  enum { endpoint_stack_size = 1 };

  struct socket_context {
    std::shared_ptr<bool> p_cancelled;
    network_address_type network_address;
    std::shared_ptr<router_type> p_router;
    typename router_type::ReceiveQueue* p_receive_queue;
  };

  struct endpoint_context_type {
    network_address_type network_addr;
    std::shared_ptr<router_type> p_router;

    bool operator<(const endpoint_context_type& rhs) const {
      return network_addr < rhs.network_addr;
    }

    bool operator!=(const endpoint_context_type& rhs) const {
      return network_addr != rhs.network_addr;
    }

    bool operator==(const endpoint_context_type& rhs) const {
      return network_addr == rhs.network_addr;
    }
  };

  using next_endpoint_type = char;

  using endpoint = basic_VirtualLink_endpoint<basic_RoutedProtocol>;
  using resolver = basic_VirtualLink_resolver<basic_RoutedProtocol>;
  using socket = boost::asio::basic_datagram_socket<
      basic_RoutedProtocol, basic_RoutedSocket_service<basic_RoutedProtocol>>;
  // using raw_socket = boost::asio::basic_datagram_socket<
  //    basic_RoutedProtocol,
  //    basic_RoutedRawSocket_service<basic_RoutedProtocol>>;

 private:
  using query = typename resolver::query;

 public:
  static endpoint make_endpoint(boost::asio::io_service& io_service,
                                typename query::const_iterator parameters_it,
                                uint32_t, boost::system::error_code& ec) {
    ec.assign(ssf::error::success, ssf::error::get_ssf_category());

    auto network_address_str =
        helpers::GetField<std::string>("network_address", *parameters_it);
    auto router_str = helpers::GetField<std::string>("router", *parameters_it);

    if (network_address_str.empty()) {
      ec.assign(ssf::error::bad_address, ssf::error::get_ssf_category());
      return endpoint();
    }

    auto p_router = get_router(router_str);
    if (!p_router) {
      ec.assign(ssf::error::identifier_removed, ssf::error::get_ssf_category());
      return endpoint();
    }

    auto network_address = ResolveToNetworkAddress(network_address_str, ec);

    if (!ec) {
      p_router->resolve(network_address, ec);
    }

    if (ec) {
      return endpoint();
    }

    endpoint_context_type context;
    context.p_router = p_router;
    context.network_addr = network_address;

    return endpoint(context, 0);
  }

  static network_address_type ResolveToNetworkAddress(
      const std::string& network_address_str, boost::system::error_code& ec) {
    try {
      auto address = std::stoul(network_address_str);
      return network_address_type(address);
    } catch (...) {
      ec.assign(ssf::error::bad_address, ssf::error::get_ssf_category());
      return network_address_type();
    }
  }

  static std::string get_address(const endpoint& endpoint) {
    return std::to_string(endpoint.endpoint_context().network_addr);
  }

  template <typename ConstBufferSequence>
  static SendDatagram make_datagram(const ConstBufferSequence& buffers,
                                    const endpoint& source,
                                    const endpoint& destination) {
    typename SendDatagram::Header dgr_header;
    auto& id = dgr_header.id();
    id = typename SendDatagram::Header::ID(
        source.endpoint_context().network_addr,
        destination.endpoint_context().network_addr);
    auto& payload_length = dgr_header.payload_length();
    payload_length = static_cast<typename SendDatagram::Header::PayloadLength>(
        boost::asio::buffer_size(buffers));

    return SendDatagram(std::move(dgr_header),
                        typename SendDatagram::Payload(buffers),
                        typename SendDatagram::Footer());
  }

  static void StartRouter(const std::string& router_name,
                          boost::asio::io_service& io_service) {
    boost::mutex::scoped_lock lock_router(router_mutex_);
    auto p_router_it = p_routers_.find(router_name);
    if (p_router_it == p_routers_.end()) {
      p_routers_.emplace(router_name,
                         std::make_shared<router_type>(io_service));
    }
  }

  static void StopRouter(const std::string& router_name,
                         boost::system::error_code& ec) {
    boost::mutex::scoped_lock lock_router(router_mutex_);
    auto p_router_it = p_routers_.find(router_name);
    if (p_router_it != p_routers_.end()) {
      p_router_it->second->close(ec);
    }
    p_routers_.erase(p_router_it);
  }

  static void StopAllRouters() {
    boost::mutex::scoped_lock lock_router(router_mutex_);
    auto p_router_it = p_routers_.begin();
    auto end_router_it = p_routers_.end();

    while (p_router_it != end_router_it) {
      boost::system::error_code ec;
      p_router_it->second->close(ec);
      p_router_it = p_routers_.erase(p_router_it);
    }
  }

  static std::shared_ptr<router_type> get_router(
      const std::string& router_name) {
    boost::mutex::scoped_lock lock_router(router_mutex_);
    auto p_router_it = p_routers_.find(router_name);
    if (p_router_it != p_routers_.end()) {
      return p_router_it->second;
    }
    return nullptr;
  }

 private:
  static boost::mutex router_mutex_;
  static std::map<std::string, std::shared_ptr<router_type>> p_routers_;
};

template <class NextLayer>
boost::mutex basic_RoutedProtocol<NextLayer>::router_mutex_;

template <class NextLayer>
std::map<std::string,
         std::shared_ptr<typename basic_RoutedProtocol<NextLayer>::router_type>>
    basic_RoutedProtocol<NextLayer>::p_routers_;

}  // routing
}  // layer
}  // ssf

#endif  // SSF_LAYER_ROUTING_BASIC_ROUTED_PROTOCOL_H_

#ifndef SSF_CORE_VIRTUAL_NETWORK_NETWORK_LAYER_BASIC_NETWORK_PROTOCOL_H_
#define SSF_CORE_VIRTUAL_NETWORK_NETWORK_LAYER_BASIC_NETWORK_PROTOCOL_H_

#include <string>
#include <set>
#include <map>

#include <boost/asio/basic_datagram_socket.hpp>
#include <boost/asio/io_service.hpp>

#include <boost/system/error_code.hpp>

#include "common/error/error.h"

#include "core/virtual_network/basic_resolver.h"
#include "core/virtual_network/basic_endpoint.h"

#include "core/virtual_network/datagram/basic_datagram.h"
#include "core/virtual_network/datagram/basic_header.h"
#include "core/virtual_network/datagram/basic_payload.h"
#include "core/virtual_network/datagram/empty_component.h"

#include "core/virtual_network/protocol_attributes.h"
#include "core/virtual_network/parameters.h"

#include "core/virtual_network/network_layer/basic_network_socket_service.h"
#include "core/virtual_network/network_layer/basic_network_raw_socket_service.h"
#include "core/virtual_network/network_layer/network_id.h"

namespace virtual_network {
namespace network_layer {

template <class NextLayer>
class basic_NetworkProtocol {
 public:
  typedef basic_Header<EmptyComponent, NetworkID, EmptyComponent, uint0_t>
      Header;
  typedef EmptyComponent Footer;

  typedef BufferPayload<NextLayer::mtu - Header::size - Footer::size>
      ReceivePayload;
  typedef basic_Datagram<Header, ReceivePayload, Footer> ReceiveDatagram;

  typedef ConstPayload SendPayload;
  typedef basic_Datagram<Header, SendPayload, Footer> SendDatagram;

  enum {
    id = 13,
    overhead = SendDatagram::size,
    facilities = virtual_network::facilities::datagram,
    mtu = NextLayer::mtu - overhead
  };
  enum { endpoint_stack_size = NextLayer::endpoint_stack_size + 1 };

  typedef NextLayer next_layer_protocol;

  struct socket_context {
    std::shared_ptr<bool> p_cancelled;
  };

  typedef NetworkID::HalfID endpoint_context_type;
  using next_endpoint_type = typename next_layer_protocol::endpoint;

  typedef basic_VirtualLink_endpoint<basic_NetworkProtocol> endpoint;
  typedef basic_VirtualLink_resolver<basic_NetworkProtocol> resolver;
  typedef boost::asio::basic_datagram_socket<
      basic_NetworkProtocol, basic_NetworkSocket_service<basic_NetworkProtocol>>
      socket;
  typedef boost::asio::basic_datagram_socket<
      basic_NetworkProtocol,
      basic_NetworkRawSocket_service<basic_NetworkProtocol>> raw_socket;

 private:
  typedef typename next_layer_protocol::endpoint next_endpoint_type;
  using query = typename resolver::query;

 public:
  static endpoint make_endpoint(boost::asio::io_service& io_service,
                                typename query::const_iterator parameters_it,
                                uint32_t, boost::system::error_code& ec) {
    endpoint_context_type context =
        NetworkID::MakeHalfID(io_service, *parameters_it, id);

    if (context) {
      return endpoint(context, next_layer_protocol::make_endpoint(
                                   io_service, ++parameters_it, id, ec));
    } else {
      ec.assign(ssf::error::bad_address, ssf::error::get_ssf_category());
      return endpoint();
    }
  }

  template <typename ConstBufferSequence>
  static SendDatagram make_datagram(const ConstBufferSequence& buffers,
                                    const endpoint& source,
                                    const endpoint& destination) {
    Header dgr_header;
    auto& id = dgr_header.id();
    id = Header::ID(source.endpoint_context(), destination.endpoint_context());

    SendPayload payload(buffers);

    return SendDatagram(dgr_header, payload, Footer());
  }

  static boost::recursive_mutex bind_mutex_;
  static std::set<next_endpoint_type> used_interface_endpoints_;
  static std::set<endpoint_context_type> bounds_;
  static std::map<endpoint_context_type, next_endpoint_type>
      network_to_interface_;
  static std::map<next_endpoint_type, endpoint_context_type>
      interface_to_network_;
};

template <class NextLayer>
boost::recursive_mutex basic_NetworkProtocol<NextLayer>::bind_mutex_;

template <class NextLayer>
std::set<typename basic_NetworkProtocol<NextLayer>::next_endpoint_type>
    basic_NetworkProtocol<NextLayer>::used_interface_endpoints_;

template <class NextLayer>
std::set<typename basic_NetworkProtocol<NextLayer>::endpoint_context_type>
    basic_NetworkProtocol<NextLayer>::bounds_;

template <class NextLayer>
std::map<typename basic_NetworkProtocol<NextLayer>::endpoint_context_type,
         typename basic_NetworkProtocol<NextLayer>::next_endpoint_type>
    basic_NetworkProtocol<NextLayer>::network_to_interface_;

template <class NextLayer>
std::map<typename basic_NetworkProtocol<NextLayer>::next_endpoint_type,
         typename basic_NetworkProtocol<NextLayer>::endpoint_context_type>
    basic_NetworkProtocol<NextLayer>::interface_to_network_;

}  // network_layer
}  // virtual_network

#endif  // SSF_CORE_VIRTUAL_NETWORK_NETWORK_LAYER_BASIC_NETWORK_PROTOCOL_H_

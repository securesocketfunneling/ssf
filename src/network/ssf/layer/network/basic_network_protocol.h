#ifndef SSF_LAYER_NETWORK_BASIC_NETWORK_PROTOCOL_H_
#define SSF_LAYER_NETWORK_BASIC_NETWORK_PROTOCOL_H_

#include <cstdint>

#include <map>
#include <mutex>
#include <set>
#include <string>

#include <boost/asio/basic_datagram_socket.hpp>
#include <boost/asio/io_service.hpp>

#include <boost/system/error_code.hpp>

#include "ssf/error/error.h"

#include "ssf/layer/basic_endpoint.h"
#include "ssf/layer/basic_resolver.h"

#include "ssf/layer/datagram/basic_datagram.h"
#include "ssf/layer/datagram/basic_header.h"
#include "ssf/layer/datagram/basic_payload.h"
#include "ssf/layer/datagram/empty_component.h"

#include "ssf/layer/parameters.h"
#include "ssf/layer/protocol_attributes.h"

#include "ssf/layer/network/basic_network_socket_service.h"
#include "ssf/layer/network/basic_network_raw_socket_service.h"
#include "ssf/layer/network/network_id.h"

namespace ssf {
namespace layer {
namespace network {

template <class NextLayer>
class basic_NetworkProtocol {
 public:
  using PayloadSize = uint16_t;
  typedef basic_Header<EmptyComponent, NetworkID, EmptyComponent, uint16_t>
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
    facilities = ssf::layer::facilities::datagram,
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
    auto& payload_length = dgr_header.payload_length();
    payload_length = boost::asio::buffer_size(buffers);

    SendPayload payload(buffers);

    return SendDatagram(dgr_header, payload, Footer());
  }

  static std::recursive_mutex bind_mutex_;
  static std::set<next_endpoint_type> used_interface_endpoints_;
  static std::set<endpoint_context_type> bounds_;
  static std::map<endpoint_context_type, next_endpoint_type>
      network_to_interface_;
  static std::map<next_endpoint_type, endpoint_context_type>
      interface_to_network_;
};

template <class NextLayer>
std::recursive_mutex basic_NetworkProtocol<NextLayer>::bind_mutex_;

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

}  // network
}  // layer
}  // ssf

#endif  // SSF_LAYER_NETWORK_BASIC_NETWORK_PROTOCOL_H_

#ifndef SSF_CORE_VIRTUAL_NETWORK_INTERFACE_LAYER_BASIC_INTERFACE_PROTOCOL_H_
#define SSF_CORE_VIRTUAL_NETWORK_INTERFACE_LAYER_BASIC_INTERFACE_PROTOCOL_H_

#include <cstdint>

#include <map>
#include <memory>
#include <limits>

#include <boost/asio/io_service.hpp>
#include <boost/asio/basic_datagram_socket.hpp>

#include <boost/system/error_code.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include "core/virtual_network/basic_impl.h"
#include "core/virtual_network/basic_resolver.h"
#include "core/virtual_network/basic_endpoint.h"

#include "core/virtual_network/interface_layer/basic_interface.h"
#include "core/virtual_network/interface_layer/basic_interface_service.h"
#include "core/virtual_network/interface_layer/basic_interface_manager.h"

#include "core/virtual_network/datagram/basic_datagram.h"
#include "core/virtual_network/datagram/basic_header.h"
#include "core/virtual_network/datagram/basic_payload.h"
#include "core/virtual_network/datagram/empty_component.h"

#include "core/virtual_network/protocol_attributes.h"
#include "core/virtual_network/parameters.h"

#include "common/utils/map_helpers.h"

#include "generic_interface_socket.h"

#define INTERFACE_MTU 65535

namespace virtual_network {
namespace interface_layer {

class basic_InterfaceProtocol {
 private:
  typedef generic_interface_socket<basic_InterfaceProtocol> socket_type;
  typedef std::shared_ptr<socket_type> p_socket_type;

 public:
  using PayloadSize = uint16_t;
  /*static_assert(std::numeric_limits<PayloadSize>::max() >= INTERFACE_MTU,
                "MTU too big for payload size field");*/
  typedef basic_Header<EmptyComponent, EmptyComponent, EmptyComponent,
                       PayloadSize> Header;
  typedef EmptyComponent Footer;

  typedef BufferPayload<INTERFACE_MTU - Header::size - Footer::size>
      ReceivePayload;
  typedef basic_Datagram<Header, ReceivePayload, Footer> ReceiveDatagram;

  typedef ConstPayload SendPayload;
  typedef basic_Datagram<Header, SendPayload, Footer> SendDatagram;

  enum {
    id = 16,
    overhead = SendDatagram::size,
    facilities = virtual_network::facilities::datagram,
    mtu = INTERFACE_MTU - overhead
  };
  enum { endpoint_stack_size = 1 };

  typedef std::string endpoint_context_type;
  using next_endpoint_type = int;

  typedef basic_InterfaceManager<basic_InterfaceProtocol>
      interface_manager_type;

  typedef basic_VirtualLink_endpoint<basic_InterfaceProtocol> endpoint;
  typedef basic_VirtualLink_resolver<basic_InterfaceProtocol> resolver;

  typedef socket_type socket;

  typedef int socket_context;

 private:
  using query = resolver::query;

 public:
  static endpoint make_endpoint(boost::asio::io_service& io_service,
                                query::const_iterator parameters_it, uint32_t,
                                boost::system::error_code& ec) {
    int next_layer_endpoint = 0;
    auto context =
        helpers::GetField<std::string>("interface_id", *parameters_it);

    return endpoint(context, next_layer_endpoint);
  }

  static p_socket_type ResolveNextLocalToGateway(
      const endpoint_context_type& next_local_endpoint) {
    boost::recursive_mutex::scoped_lock lock(get_interface_manager().mutex);
    auto p_next_socket_it =
        get_interface_manager().available_sockets.find(next_local_endpoint);

    if (p_next_socket_it ==
        std::end(get_interface_manager().available_sockets)) {
      return nullptr;
    }

    return p_next_socket_it->second;
  }

  template <typename ConstBufferSequence>
  static SendDatagram make_datagram(const ConstBufferSequence& buffers) {
    Header dgr_header;
    auto& data_size = dgr_header.payload_length();
    data_size =
        static_cast<Header::PayloadLength>(boost::asio::buffer_size(buffers));
    SendPayload payload(buffers);

    return SendDatagram(dgr_header, payload, Footer());
  }

  static interface_manager_type& get_interface_manager() {
    return interface_manager_;
  }

 private:
  static interface_manager_type interface_manager_;
};

}  // interface_layer
}  // virtual_network

#endif  // SSF_CORE_VIRTUAL_NETWORK_INTERFACE_LAYER_BASIC_INTERFACE_PROTOCOL_H_
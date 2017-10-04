#ifndef SSF_LAYER_INTERFACE_LAYER_BASIC_INTERFACE_PROTOCOL_H_
#define SSF_LAYER_INTERFACE_LAYER_BASIC_INTERFACE_PROTOCOL_H_

#include <cstdint>

#include <limits>
#include <map>
#include <memory>

#include <boost/asio/basic_datagram_socket.hpp>
#include <boost/asio/io_service.hpp>

#include <boost/system/error_code.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include "ssf/layer/basic_endpoint.h"
#include "ssf/layer/basic_impl.h"
#include "ssf/layer/basic_resolver.h"

#include "ssf/layer/interface_layer/basic_interface.h"
#include "ssf/layer/interface_layer/basic_interface_service.h"
#include "ssf/layer/interface_layer/basic_interface_manager.h"

#include "ssf/layer/datagram/basic_datagram.h"
#include "ssf/layer/datagram/basic_header.h"
#include "ssf/layer/datagram/basic_payload.h"
#include "ssf/layer/datagram/empty_component.h"

#include "ssf/layer/protocol_attributes.h"
#include "ssf/layer/parameters.h"

#include "ssf/utils/map_helpers.h"

#include "ssf/layer/interface_layer/generic_interface_socket.h"

#define INTERFACE_MTU 65535

namespace ssf {
namespace layer {
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
    facilities = ssf::layer::facilities::datagram,
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
    auto p_next_socket_optional = get_interface_manager().Find(next_local_endpoint);

    if (!p_next_socket_optional) {
      return nullptr;
    }

    return *p_next_socket_optional;
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
}  // layer
}  // ssf

#endif  // SSF_LAYER_INTERFACE_LAYER_BASIC_INTERFACE_PROTOCOL_H_
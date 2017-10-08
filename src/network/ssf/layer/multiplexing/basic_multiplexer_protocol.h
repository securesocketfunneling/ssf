#ifndef SSF_LAYER_MULTIPLEXING_BASIC_MULTIPLEXER_PROTOCOL_H_
#define SSF_LAYER_MULTIPLEXING_BASIC_MULTIPLEXER_PROTOCOL_H_

#include <cstdint>

#include <map>
#include <mutex>
#include <queue>
#include <set>

#include <boost/asio/basic_datagram_socket.hpp>
#include <boost/asio/detail/op_queue.hpp>
#include <boost/asio/io_service.hpp>

#include <boost/system/error_code.hpp>

#include "ssf/io/read_op.h"

#include "ssf/layer/basic_endpoint.h"
#include "ssf/layer/basic_resolver.h"

#include "ssf/layer/parameters.h"
#include "ssf/layer/protocol_attributes.h"

#include "ssf/layer/datagram/basic_datagram.h"
#include "ssf/layer/datagram/basic_header.h"
#include "ssf/layer/datagram/basic_payload.h"
#include "ssf/layer/datagram/empty_component.h"

#include "ssf/layer/multiplexing/basic_multiplexer_socket_service.h"

#include "ssf/utils/map_helpers.h"

namespace ssf {
namespace layer {
namespace multiplexing {

template <class NextLayer, class MultiplexID, class CongestionPolicy>
class basic_MultiplexedProtocol {
 private:
  typedef typename NextLayer::socket next_socket_type;

 public:
  typedef MultiplexID id_type;
  typedef basic_Header<EmptyComponent, typename id_type::FullID, EmptyComponent,
                       uint0_t>
      Header;
  typedef EmptyComponent Footer;

  typedef BufferPayload<NextLayer::mtu - Header::size - Footer::size>
      ReceivePayload;
  typedef basic_Datagram<Header, ReceivePayload, Footer> ReceiveDatagram;

  typedef ConstPayload SendPayload;
  typedef basic_Datagram<Header, SendPayload, Footer> SendDatagram;

  enum {
    id = 16,
    overhead = SendDatagram::size,
    facilities = ssf::layer::facilities::datagram,
    mtu = NextLayer::mtu - overhead
  };
  enum { endpoint_stack_size = NextLayer::endpoint_stack_size + 1 };

  typedef NextLayer next_layer_protocol;
  typedef typename id_type::HalfID endpoint_context_type;
  using next_endpoint_type = typename next_layer_protocol::endpoint;

  typedef CongestionPolicy congestion_policy_type;

  typedef basic_VirtualLink_endpoint<basic_MultiplexedProtocol> endpoint;
  typedef basic_VirtualLink_resolver<basic_MultiplexedProtocol> resolver;
  typedef boost::asio::basic_datagram_socket<
      basic_MultiplexedProtocol,
      basic_MultiplexedSocket_service<basic_MultiplexedProtocol>>
      socket;

  struct socket_context {
    std::recursive_mutex mutex;

    endpoint_context_type local_id;
    endpoint_context_type remote_id;

    std::queue<ReceiveDatagram> datagram_queue;

    std::queue<typename next_layer_protocol::endpoint> next_endpoint_queue;

    boost::asio::detail::op_queue<
        io::basic_pending_read_operation<basic_MultiplexedProtocol>>
        read_op_queue;
  };

 private:
  using query = typename resolver::query;

 public:
  static endpoint make_endpoint(boost::asio::io_service& io_service,
                                typename query::const_iterator parameters_it,
                                uint32_t id, boost::system::error_code& ec) {
    auto context = id_type::MakeHalfID(io_service, *parameters_it, id);
    if (!id) {
      id = basic_MultiplexedProtocol::id;
    }

    auto next_layer_endpoint =
        next_layer_protocol::make_endpoint(io_service, ++parameters_it, id, ec);

    return endpoint(context, next_layer_endpoint);
  }

  static endpoint remote_to_local_endpoint(
      const endpoint& remote_endpoint,
      const std::set<endpoint>& used_local_endpoints) {
    std::set<next_endpoint_type> used_local_next_endpoints;
    for (const auto& endpoint : used_local_endpoints) {
      used_local_next_endpoints.insert(endpoint.next_layer_endpoint());
    }

    auto next_endpoint = next_layer_protocol::remote_to_local_endpoint(
        remote_endpoint.next_layer_endpoint(), used_local_next_endpoints);

    if (next_endpoint == next_layer_protocol::endpoint()) {
      return endpoint();
    }

    auto context = FindAvailableContext(next_endpoint, remote_endpoint,
                                        used_local_endpoints);

    return endpoint(context, next_endpoint);
  }

  template <typename ConstBufferSequence>
  static SendDatagram make_datagram(const ConstBufferSequence& buffers,
                                    const endpoint& source,
                                    const endpoint& destination) {
    Header dgr_header;
    auto& id = dgr_header.id();
    id = Header::ID(source.endpoint_context(), destination.endpoint_context());
    /*auto& data_size = dgr_header.payload_length();
    data_size = boost::asio::buffer_size(buffers);*/
    SendPayload payload(buffers);

    return SendDatagram(dgr_header, payload, Footer());
  }

 private:
  static endpoint_context_type FindAvailableContext(
      const next_endpoint_type& next_endpoint, const endpoint& remote_endpoint,
      const std::set<endpoint>& used_local_endpoints) {
    std::set<endpoint_context_type> context_set;

    // Find local endpoint contexts whose next layer is identic to
    // the remote next layer endpoint context
    for (const auto& endpoint : used_local_endpoints) {
      if (endpoint.next_layer_endpoint() == next_endpoint) {
        context_set.insert(endpoint.endpoint_context());
      }
    }

    return id_type::LocalHalfIDFromRemote(remote_endpoint.endpoint_context(),
                                          context_set);
  }
};

}  // multiplexing
}  // layer
}  // ssf

#endif  // SSF_LAYER_MULTIPLEXING_BASIC_MULTIPLEXER_PROTOCOL_H_

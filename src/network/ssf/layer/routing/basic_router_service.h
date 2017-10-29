#ifndef SSF_LAYER_ROUTING_BASIC_ROUTER_SERVICE_H_
#define SSF_LAYER_ROUTING_BASIC_ROUTER_SERVICE_H_

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>

#include <boost/asio/async_result.hpp>
#include <boost/asio/handler_type.hpp>
#include <boost/asio/io_service.hpp>

#include <boost/system/error_code.hpp>

#include "ssf/error/error.h"
#include "ssf/io/composed_op.h"
#include "ssf/io/handler_helpers.h"

#include "ssf/layer/protocol_attributes.h"

#include "ssf/layer/queue/async_queue.h"
#include "ssf/layer/queue/commutator.h"
#include "ssf/layer/queue/send_queued_datagram_socket.h"

#include "ssf/layer/routing/basic_routing_selector.h"

#include "ssf/log/log.h"

namespace ssf {
namespace layer {
namespace routing {

#include <boost/asio/detail/push_options.hpp>

template <class NetworkProtocol, class TRoutingTable>
class basic_Router_service
    : public boost::asio::detail::service_base<
          basic_Router_service<NetworkProtocol, TRoutingTable>> {
 public:
  typedef NetworkProtocol next_layer_protocol;
  typedef typename next_layer_protocol::SendDatagram SendDatagram;
  typedef typename next_layer_protocol::ReceiveDatagram Datagram;
  typedef typename next_layer_protocol::endpoint next_endpoint_type;

  typedef
      typename next_layer_protocol::endpoint_context_type network_address_type;
  typedef network_address_type prefix_type;

  typedef RoutingSelector<network_address_type, Datagram, TRoutingTable>
      routing_selector_type;
  typedef queue::Commutator<network_address_type, Datagram,
                            routing_selector_type>
      Commutator;
  typedef queue::CommutatorPtr<network_address_type, Datagram,
                               routing_selector_type>
      CommutatorPtr;

  typedef typename next_layer_protocol::raw_socket next_socket_type;
  typedef queue::basic_send_queued_datagram_socket<next_socket_type>
      queued_send_socket_type;
  typedef std::shared_ptr<queued_send_socket_type> p_queued_send_socket_type;

  typedef TRoutingTable RoutingTable;

  using ReceiveQueue = queue::basic_async_queue<Datagram>;
  using SendQueue = queue::basic_async_queue<Datagram>;

 public:
  struct implementation_type {
    std::shared_ptr<bool> p_valid;
    RoutingTable routing_table;

    std::recursive_mutex network_sockets_mutex;
    std::map<network_address_type, p_queued_send_socket_type> network_sockets;
    std::map<network_address_type, ReceiveQueue> receive_queues;

    std::unique_ptr<SendQueue> p_send_queue;

    routing_selector_type selector;
    CommutatorPtr p_commutator;

    std::vector<uint8_t> local_send_buffer;
  };

  typedef implementation_type& native_handle_type;
  typedef native_handle_type native_type;

 public:
  explicit basic_Router_service(boost::asio::io_service& io_service)
      : boost::asio::detail::service_base<basic_Router_service>(io_service) {}

  virtual ~basic_Router_service() {}

  void construct(implementation_type& impl) {
    impl.p_valid = std::make_shared<bool>(true);
    impl.selector.set_routing_table(&impl.routing_table);

    impl.p_send_queue =
        std::unique_ptr<SendQueue>(new SendQueue(this->get_io_service()));

    impl.p_commutator =
        Commutator::Create(this->get_io_service(), impl.selector);

    auto receive_callback = [this, &impl](
        typename Commutator::Element datagram,
        typename Commutator::OutputHandler handler) {
      boost::system::error_code receive_ec;
      this->Receive(impl, std::move(datagram), receive_ec);
      handler(receive_ec);
    };

    /// Register local output (0) : final destination
    /// Populate local receive destination queue
    impl.p_commutator->RegisterOutput(0, std::move(receive_callback));

    impl.local_send_buffer.resize(next_layer_protocol::mtu);

    auto send_callback = [this,
                          &impl](typename Commutator::InputHandler handler) {
      this->AsyncSend(impl, std::move(handler));
    };

    /// Register local input : local send
    /// Get from send local queue
    /// Push element to destination output
    impl.p_commutator->RegisterInput(0, std::move(send_callback));
  }

  void destroy(implementation_type& impl) {
    *(impl.p_valid) = false;

    boost::system::error_code queue_ec;
    impl.p_send_queue->close(queue_ec);
    impl.p_send_queue.reset();

    {
      std::unique_lock<std::recursive_mutex> lock(impl.network_sockets_mutex);
      boost::system::error_code close_ec;

      for (auto& p_sockets : impl.network_sockets) {
        p_sockets.second->close(close_ec);
      }
      impl.network_sockets.clear();

      for (auto& queue : impl.receive_queues) {
        queue.second.close(close_ec);
      }
      impl.receive_queues.clear();
    }

    boost::system::error_code ec;
    impl.routing_table.Flush(ec);
  }

  void move_construct(implementation_type& impl, implementation_type& other) {
    impl = std::move(other);
  }

  void move_assign(implementation_type& impl, implementation_type& other) {
    impl = std::move(other);
  }

  /// Pull the network socket bound to the next endpoint
  ///   and dispatch the received packet :
  ///     * locally if the destination id is local
  ///     * remotely by sending it through the correct network socket
  ///       (routing table)
  /// Declare the network id output to send packet through the network socket
  boost::system::error_code add_network(implementation_type& impl,
                                        prefix_type prefix,
                                        next_endpoint_type next_endpoint,
                                        boost::system::error_code& ec) {
    auto p_queued_send_socket =
        std::make_shared<queued_send_socket_type>(this->get_io_service());
    p_queued_send_socket->next_layer().bind(next_endpoint, ec);

    std::unique_lock<std::recursive_mutex> lock(impl.network_sockets_mutex);

    SSF_LOG("network_router", trace, "add network {} bound to interface {}",
            prefix, next_endpoint.next_layer_endpoint().endpoint_context());

    if (!ec) {
      auto inserted = impl.network_sockets.insert(std::make_pair(
          next_endpoint.endpoint_context(), p_queued_send_socket));

      if (!inserted.second) {
        ec.assign(ssf::error::address_in_use, ssf::error::get_ssf_category());
      }
    }

    if (!ec) {
      auto received_queue_it =
          impl.receive_queues.find(next_endpoint.endpoint_context());
      if (received_queue_it == impl.receive_queues.end()) {
        // The queue does not exists, create it
        auto inserted = impl.receive_queues.emplace(
            next_endpoint.endpoint_context(), this->get_io_service());

        if (!inserted.second) {
          impl.network_sockets.erase(next_endpoint.endpoint_context());
          ec.assign(ssf::error::address_in_use, ssf::error::get_ssf_category());
        }
      }
    }

    if (!ec) {
      /// Register network as local in routing table
      impl.routing_table.AddRoute(next_endpoint.endpoint_context(),
                                  network_address_type(), ec);

      if (ec) {
        impl.network_sockets.erase(next_endpoint.endpoint_context());
        impl.receive_queues.erase(next_endpoint.endpoint_context());
        boost::system::error_code close_ec;
        p_queued_send_socket->close(close_ec);
      }
    }

    if (!ec) {
      auto output_callback = [p_queued_send_socket](
          typename Commutator::Element datagram,
          typename Commutator::OutputHandler handler) {
        auto p_datagram = std::make_shared<Datagram>(std::move(datagram));

        auto complete_handler = [p_datagram, handler, p_queued_send_socket](
            const boost::system::error_code& ec, std::size_t) { handler(ec); };

        AsyncSendDatagram(*p_queued_send_socket, *p_datagram,
                          std::move(complete_handler));
      };

      auto input_callback =
          [p_queued_send_socket](typename Commutator::InputHandler handler) {
            auto p_datagram = std::make_shared<Datagram>();

            auto complete_handler = [p_datagram, handler, p_queued_send_socket](
                const boost::system::error_code& ec, std::size_t) {
              handler(ec, std::move(*p_datagram));
            };

            AsyncReceiveDatagram(*p_queued_send_socket, p_datagram.get(),
                                 std::move(complete_handler));
          };

      /// Register network socket as output channel
      impl.p_commutator->RegisterOutput(next_endpoint.endpoint_context(),
                                        std::move(output_callback));
      /// Pull network socket
      impl.p_commutator->RegisterInput(prefix, std::move(input_callback));
    }

    return ec;
  }

  template <class Handler>
  BOOST_ASIO_INITFN_RESULT_TYPE(Handler, void(boost::system::error_code))
  async_add_network(implementation_type& impl, const prefix_type& prefix,
                    const next_endpoint_type& next_endpoint,
                    Handler&& handler) {
    boost::asio::detail::async_result_init<Handler,
                                           void(boost::system::error_code)>
        init(std::forward<Handler>(handler));

    auto complete_lambda = [this, &impl, prefix, next_endpoint, init]() {
      boost::system::error_code ec;

      this->add_network(impl, prefix, next_endpoint, ec);

      init.handler(ec);
    };

    this->get_io_service().post(std::move(complete_lambda));

    return init.result.get();
  }

  boost::system::error_code remove_network(implementation_type& impl,
                                           const prefix_type& prefix,
                                           boost::system::error_code& ec) {
    impl.p_commutator->UnregisterInput(prefix);
    impl.p_commutator->UnregisterOutput(prefix);

    auto network_address = impl.routing_table.Resolve(prefix, ec);

    if (ec) {
      return ec;
    }

    impl.routing_table.RemoveRoute(network_address, ec);

    std::unique_lock<std::recursive_mutex> lock(impl.network_sockets_mutex);

    auto p_socket_it = impl.network_sockets.find(network_address);

    if (p_socket_it != std::end(impl.network_sockets)) {
      boost::system::error_code close_ec;
      p_socket_it->second->close(close_ec);
      impl.network_sockets.erase(network_address);
    }

    auto queue_it = impl.receive_queues.find(network_address);

    if (queue_it != std::end(impl.receive_queues)) {
      boost::system::error_code close_ec;
      queue_it->second.close(close_ec);
      impl.receive_queues.erase(network_address);
    }

    return ec;
  }

  /// Get local network receive queue
  // TODO : care about lifetime
  ReceiveQueue* get_network_receive_queue(implementation_type& impl,
                                          prefix_type prefix,
                                          boost::system::error_code& ec) {
    std::unique_lock<std::recursive_mutex> lock(impl.network_sockets_mutex);

    auto queue_it = impl.receive_queues.find(prefix);

    // The queue does not exists (interface not up yet ?)
    if (queue_it == std::end(impl.receive_queues)) {
      // Create the receive queue
      auto inserted =
          impl.receive_queues.emplace(prefix, this->get_io_service());
      if (inserted.second) {
        return &(inserted.first->second);
      }
      return nullptr;
    }

    return &queue_it->second;
  }

  SendQueue* get_router_send_queue(implementation_type& impl,
                                   boost::system::error_code& ec) {
    return impl.p_send_queue.get();
  }

  boost::system::error_code add_route(
      implementation_type& impl, prefix_type prefix,
      network_address_type next_endpoint_context,
      boost::system::error_code& ec) {
    return impl.routing_table.AddRoute(std::move(prefix),
                                       std::move(next_endpoint_context), ec);
  }

  boost::system::error_code remove_route(implementation_type& impl,
                                         prefix_type prefix,
                                         boost::system::error_code& ec) {
    return impl.routing_table.RemoveRoute(prefix, ec);
  }

  network_address_type resolve(const implementation_type& impl,
                               const prefix_type& prefix,
                               boost::system::error_code& ec) const {
    return impl.routing_table.Resolve(prefix, ec);
  }

  template <class Handler>
  BOOST_ASIO_INITFN_RESULT_TYPE(Handler, void(const boost::system::error_code&,
                                              const network_address_type&))
  async_resolve(const implementation_type& impl, const prefix_type& prefix,
                Handler&& handler) {
    boost::asio::detail::async_result_init<
        Handler,
        void(const boost::system::error_code&, const network_address_type&)>
        init(std::forward<Handler>(handler));

    auto complete_lambda = [this, &impl, prefix, init]() {
      boost::system::error_code ec;

      auto resolved = this->resolve(impl, prefix, ec);

      init.handler(ec, resolved);
    };

    this->get_io_service().post(std::move(complete_lambda));

    return init.result.get();
  }

  /// Async send packet
  /// Push packet in the local snd queue
  template <class Handler>
  BOOST_ASIO_INITFN_RESULT_TYPE(Handler, void(const boost::system::error_code&,
                                              std::size_t))
  async_send(implementation_type& impl, const SendDatagram& datagram,
             Handler&& handler) {
    boost::asio::detail::async_result_init<
        Handler, void(const boost::system::error_code&, std::size_t)>
        init(std::forward<Handler>(handler));

    Datagram copied_datagram;

    auto& payload_length = copied_datagram.header().payload_length();
    payload_length = datagram.header().payload_length();

    copied_datagram.payload().SetSize(payload_length);
    auto copied = boost::asio::buffer_copy(copied_datagram.GetMutableBuffers(),
                                           datagram.GetConstBuffers());

    auto push_handler = [this, copied,
                         init](const boost::system::error_code ec) {
      if (ec) {
        io::PostHandler(this->get_io_service(), init.handler, ec, 0);
      } else {
        io::PostHandler(this->get_io_service(), init.handler, ec, copied);
      }
    };

    impl.p_send_queue->async_push(std::move(copied_datagram), push_handler);

    return init.result.get();
  }

  boost::system::error_code flush(implementation_type& impl,
                                  boost::system::error_code& ec) {
    return impl.routing_table.Flush(ec);
  }

  boost::system::error_code close(implementation_type& impl,
                                  boost::system::error_code& ec) {
    impl.p_send_queue->close(ec);

    impl.p_commutator->close(ec);
    impl.routing_table.Flush(ec);

    std::unique_lock<std::recursive_mutex> lock(impl.network_sockets_mutex);
    boost::system::error_code close_ec;

    for (auto& p_sockets : impl.network_sockets) {
      p_sockets.second->close(close_ec);
    }
    impl.network_sockets.clear();

    for (auto& queue : impl.receive_queues) {
      queue.second.close(close_ec);
    }
    impl.receive_queues.clear();

    return ec;
  }

 private:
  void shutdown_service() {}

  void AsyncSend(implementation_type& impl,
                 typename Commutator::InputHandler handler) {
    impl.p_send_queue->async_get(std::move(handler));
  }

  /// A packet is received locally
  /// Find the receive queue bound to the destination and push the datagram in
  boost::system::error_code Receive(implementation_type& impl,
                                    Datagram datagram,
                                    boost::system::error_code& ec) {
    std::unique_lock<std::recursive_mutex> lock(impl.network_sockets_mutex);

    auto& destination = datagram.header().id().right_id();

    auto queue_it = impl.receive_queues.find(destination);

    if (queue_it == std::end(impl.receive_queues)) {
      ec.assign(ssf::error::address_not_available,
                ssf::error::get_ssf_category());
      return ec;
    }

    queue_it->second.push(std::move(datagram), ec);

    return ec;
  }
};

#include <boost/asio/detail/pop_options.hpp>

}  // routing
}  // layer
}  // ssf

#endif  // SSF_LAYER_ROUTING_BASIC_ROUTER_SERVICE_H_

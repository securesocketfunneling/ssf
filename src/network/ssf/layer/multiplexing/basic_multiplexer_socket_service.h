#ifndef SSF_LAYER_MULTIPLEXING_BASIC_MULTIPLEXER_SOCKET_SERVICE_H_
#define SSF_LAYER_MULTIPLEXING_BASIC_MULTIPLEXER_SOCKET_SERVICE_H_

#include <cstdint>

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>

#include <boost/system/error_code.hpp>

#include <boost/asio/detail/op_queue.hpp>
#include <boost/asio/io_service.hpp>

#include "ssf/error/error.h"
#include "ssf/io/composed_op.h"
#include "ssf/io/read_op.h"

#include "ssf/layer/basic_impl.h"

#include "ssf/layer/protocol_attributes.h"

#include "ssf/layer/multiplexing/basic_demultiplexer.h"
#include "ssf/layer/multiplexing/basic_multiplexer.h"
#include "ssf/layer/multiplexing/demultiplexer_manager.h"
#include "ssf/layer/multiplexing/multiplexer_manager.h"

namespace ssf {
namespace layer {
namespace multiplexing {

#include <boost/asio/detail/push_options.hpp>

template <class Protocol>
class basic_MultiplexedSocket_service
    : public boost::asio::detail::service_base<
          basic_MultiplexedSocket_service<Protocol>> {
 public:
  /// The protocol type.
  typedef Protocol protocol_type;
  /// The endpoint type.
  typedef typename protocol_type::endpoint endpoint_type;
  typedef typename protocol_type::resolver resolver_type;
  typedef typename protocol_type::id_type id_type;
  typedef typename protocol_type::SendDatagram send_datagram_type;
  typedef typename protocol_type::ReceiveDatagram receive_datagram_type;
  typedef std::shared_ptr<receive_datagram_type> p_receive_datagram_type;

  typedef basic_socket_impl<protocol_type> implementation_type;
  typedef implementation_type& native_handle_type;
  typedef native_handle_type native_type;

 private:
  typedef typename protocol_type::next_layer_protocol::socket next_socket_type;
  typedef std::shared_ptr<next_socket_type> p_next_socket_type;
  typedef
      typename protocol_type::next_layer_protocol::endpoint next_endpoint_type;
  typedef std::shared_ptr<next_endpoint_type> p_next_endpoint_type;
  typedef typename protocol_type::endpoint_context_type endpoint_context_type;
  typedef typename protocol_type::socket_context socket_context_type;
  typedef std::shared_ptr<socket_context_type> p_socket_context_type;
  typedef typename protocol_type::congestion_policy_type congestion_policy_type;

 public:
  explicit basic_MultiplexedSocket_service(boost::asio::io_service& io_service)
      : boost::asio::detail::service_base<basic_MultiplexedSocket_service>(
            io_service) {}

  virtual ~basic_MultiplexedSocket_service() {}

  void construct(implementation_type& impl) {}

  void destroy(implementation_type& impl) {
    impl.p_socket_context.reset();
    impl.p_local_endpoint.reset();
    impl.p_remote_endpoint.reset();
    impl.p_next_layer_socket.reset();
  }

  void move_construct(implementation_type& impl, implementation_type& other) {
    impl = std::move(other);
  }

  void move_assign(implementation_type& impl, implementation_type& other) {
    impl = std::move(other);
  }

  boost::system::error_code open(implementation_type& impl,
                                 const protocol_type& protocol,
                                 boost::system::error_code& ec) {
    return ec;
  }

  boost::system::error_code assign(implementation_type& impl,
                                   const protocol_type& protocol,
                                   native_handle_type& native_socket,
                                   boost::system::error_code& ec) {
    impl = native_socket;

    return ec;
  }

  bool is_open(const implementation_type& impl) const {
    return !!impl.p_next_layer_socket;
  }

  endpoint_type remote_endpoint(const implementation_type& impl,
                                boost::system::error_code& ec) const {
    if (impl.p_remote_endpoint) {
      return *impl.p_remote_endpoint;
    } else {
      return endpoint_type();
    }
  }

  endpoint_type local_endpoint(const implementation_type& impl,
                               boost::system::error_code& ec) const {
    if (impl.p_local_endpoint) {
      return *impl.p_local_endpoint;
    } else {
      return endpoint_type();
    }
  }

  boost::system::error_code close(implementation_type& impl,
                                  boost::system::error_code& ec) {
    {
      std::unique_lock<std::recursive_mutex> lock(mutex_);

      // Unbind context in demultiplexer
      demultiplexer_manager_.Unbind(impl.p_next_layer_socket,
                                    impl.p_socket_context);

      if (impl.p_local_endpoint) {
        local_endpoints_.erase(*(impl.p_local_endpoint));
        auto& next_endpoint = impl.p_local_endpoint->next_layer_endpoint();

        auto pair_p_next_socket_it =
            next_endpoint_to_next_socket_.find(next_endpoint);

        if (pair_p_next_socket_it != std::end(next_endpoint_to_next_socket_)) {
          // Decrease usage counter
          --(pair_p_next_socket_it->second.second);

          if (!pair_p_next_socket_it->second.second) {
            // Next socket layer unused
            StopMultiplexer(pair_p_next_socket_it->second.first);
            StopDemultiplexer(pair_p_next_socket_it->second.first);
            next_endpoint_to_next_socket_.erase(next_endpoint);
            impl.p_next_layer_socket->close(ec);
          }
        }
      }
    }
    impl.p_socket_context.reset();
    impl.p_local_endpoint.reset();
    impl.p_remote_endpoint.reset();
    impl.p_next_layer_socket.reset();

    return ec;
  }

  native_type native(implementation_type& impl) { return impl; }

  native_handle_type native_handle(implementation_type& impl) { return impl; }

  bool at_mark(const implementation_type& impl,
               boost::system::error_code& ec) const {
    if (!impl.p_next_layer_socket) {
      return false;
    }

    return impl.p_next_layer_socket->at_mark(ec);
  }

  /// Size in bytes of the next datagram payload
  std::size_t available(const implementation_type& impl,
                        boost::system::error_code& ec) const {
    if (!impl.p_socket_context) {
      ec.assign(ssf::error::bad_file_descriptor,
                ssf::error::get_ssf_category());
      return 0;
    }

    std::unique_lock<std::recursive_mutex> lock(impl.p_socket_context->mutex);
    ec.assign(ssf::error::success, ssf::error::get_ssf_category());
    if (impl.p_socket_context->datagram_queue.empty()) {
      return 0;
    }

    return impl.p_socket_context->datagram_queue.front().payload().GetSize();
  }

  boost::system::error_code bind(implementation_type& impl,
                                 endpoint_type local_endpoint,
                                 boost::system::error_code& ec) {
    std::unique_lock<std::recursive_mutex> lock(mutex_);

    // Get the next layer socket which is linked to the next local endpoint
    auto pair_p_next_socket_it = next_endpoint_to_next_socket_.find(
        local_endpoint.next_layer_endpoint());

    if (pair_p_next_socket_it == std::end(next_endpoint_to_next_socket_)) {
      // Next layer endpoint unknown => create a new socket for this endpoint,
      // create its multiplexer and start it
      impl.p_next_layer_socket =
          std::make_shared<next_socket_type>(this->get_io_service());
      impl.p_next_layer_socket->bind(local_endpoint.next_layer_endpoint(), ec);

      if (ec) {
        return ec;
      }
      auto next_endpoint_inserted = next_endpoint_to_next_socket_.emplace(
          local_endpoint.next_layer_endpoint(),
          std::make_pair(impl.p_next_layer_socket, 1));

      StartDemultiplexer(impl.p_next_layer_socket);
      StartMultiplexer(impl.p_next_layer_socket);
    } else {
      // Increase usage counter
      ++(pair_p_next_socket_it->second.second);
      impl.p_next_layer_socket = pair_p_next_socket_it->second.first;
    }

    if (!impl.p_socket_context) {
      impl.p_socket_context =
          std::make_shared<typename protocol_type::socket_context>();
    }

    impl.p_socket_context->local_id = local_endpoint.endpoint_context();

    if (!demultiplexer_manager_.Bind(impl.p_next_layer_socket,
                                     impl.p_socket_context)) {
      ec.assign(ssf::error::address_in_use, ssf::error::get_ssf_category());
      return ec;
    }
    local_endpoints_.insert(local_endpoint);
    impl.p_local_endpoint =
        std::make_shared<endpoint_type>(std::move(local_endpoint));
    local_endpoints_.insert(local_endpoint);
    return ec;
  }

  boost::system::error_code connect(implementation_type& impl,
                                    endpoint_type remote_endpoint,
                                    boost::system::error_code& ec) {
    std::unique_lock<std::recursive_mutex> lock(mutex_);

    if (!impl.p_socket_context) {
      impl.p_socket_context =
          std::make_shared<typename protocol_type::socket_context>();
    }

    if (!impl.p_local_endpoint) {
      impl.p_remote_endpoint =
          std::make_shared<endpoint_type>(std::move(remote_endpoint));
      impl.p_socket_context->remote_id = remote_endpoint.endpoint_context();

      auto local_endpoint = protocol_type::remote_to_local_endpoint(
          remote_endpoint, local_endpoints_);

      if (local_endpoint == endpoint_type()) {
        ec.assign(ssf::error::address_not_available,
                  ssf::error::get_ssf_category());
        return ec;
      }

      bind(impl, std::move(local_endpoint), ec);
    } else {
      if (!impl.p_remote_endpoint) {
        impl.p_remote_endpoint = std::make_shared<endpoint_type>();
      }

      auto binding_changed = ChangeBinding(impl, remote_endpoint);

      if (!binding_changed) {
        ec.assign(ssf::error::no_link, ssf::error::get_ssf_category());
        return ec;
      }
    }

    return ec;
  }

  template <typename ConnectHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(ConnectHandler, void(boost::system::error_code))
  async_connect(implementation_type& impl, const endpoint_type& remote_endpoint,
                ConnectHandler&& handler) {
    boost::asio::detail::async_result_init<
        ConnectHandler, void(const boost::system::error_code&)>
        init(std::forward<ConnectHandler>(handler));

    auto connect_lambda = [this, &impl, remote_endpoint, init]() {
      boost::system::error_code ec;
      this->connect(impl, remote_endpoint, ec);
      init.handler(ec);
    };

    this->get_io_service().post(
        io::ComposedOp<decltype(connect_lambda), ConnectHandler>(
            std::move(connect_lambda), init.handler));

    return init.result.get();
  }

  template <typename ConstBufferSequence, typename WriteHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler,
                                void(boost::system::error_code, std::size_t))
  async_send(implementation_type& impl, const ConstBufferSequence& buffers,
             boost::asio::socket_base::message_flags flags,
             WriteHandler&& handler) {
    boost::asio::detail::async_result_init<
        WriteHandler, void(boost::system::error_code, std::size_t)>
        init(std::forward<WriteHandler>(handler));

    if (!impl.p_remote_endpoint) {
      this->get_io_service().post(
          boost::asio::detail::binder2<decltype(init.handler),
                                       boost::system::error_code, std::size_t>(
              init.handler, boost::system::error_code(
                                ssf::error::destination_address_required,
                                ssf::error::get_ssf_category()),
              0));
      return init.result.get();
    }

    return async_send_to(impl, buffers, *impl.p_remote_endpoint, flags,
                         init.handler);
  }

  template <typename ConstBufferSequence, typename WriteHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler,
                                void(boost::system::error_code, std::size_t))
  async_send_to(implementation_type& impl, const ConstBufferSequence& buffers,
                const endpoint_type& destination,
                boost::asio::socket_base::message_flags flags,
                WriteHandler&& handler) {
    boost::asio::detail::async_result_init<
        WriteHandler, void(boost::system::error_code, std::size_t)>
        init(std::forward<WriteHandler>(handler));

    if (!impl.p_next_layer_socket || !impl.p_socket_context ||
        !impl.p_local_endpoint) {
      this->get_io_service().post(
          boost::asio::detail::binder2<decltype(init.handler),
                                       boost::system::error_code, std::size_t>(
              init.handler, boost::system::error_code(
                                ssf::error::destination_address_required,
                                ssf::error::get_ssf_category()),
              0));
      return init.result.get();
    }

    if (boost::asio::buffer_size(buffers) > protocol_type::mtu) {
      this->get_io_service().post(
          boost::asio::detail::binder2<decltype(init.handler),
                                       boost::system::error_code, std::size_t>(
              init.handler,
              boost::system::error_code(ssf::error::message_size,
                                        ssf::error::get_ssf_category()),
              0));
      return init.result.get();
    }

    auto datagram = protocol_type::make_datagram(
        buffers, *impl.p_local_endpoint, destination);

    auto complete_lambda = [init](const boost::system::error_code& ec,
                                  std::size_t length) {
      if (length < send_datagram_type::size) {
        init.handler(boost::system::error_code(ssf::error::broken_pipe,
                                               ssf::error::get_ssf_category()),
                     0);
        return;
      }

      init.handler(ec, length - send_datagram_type::size);
    };

    multiplexer_manager_.Send(
        impl.p_next_layer_socket, std::move(datagram),
        destination.next_layer_endpoint(),
        io::ComposedOp<decltype(complete_lambda), decltype(init.handler)>(
            std::move(complete_lambda), init.handler));

    return init.result.get();
  }

  template <typename MutableBufferSequence, typename ReadHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler,
                                void(boost::system::error_code, std::size_t))
  async_receive(implementation_type& impl, const MutableBufferSequence& buffers,
                boost::asio::socket_base::message_flags flags,
                ReadHandler&& handler) {
    boost::asio::detail::async_result_init<
        ReadHandler, void(boost::system::error_code, std::size_t)>
        init(std::forward<ReadHandler>(handler));

    if (!impl.p_remote_endpoint) {
      this->get_io_service().post(
          boost::asio::detail::binder2<decltype(init.handler),
                                       boost::system::error_code, std::size_t>(
              init.handler, boost::system::error_code(
                                ssf::error::destination_address_required,
                                ssf::error::get_ssf_category()),
              0));
      return init.result.get();
    }

    return async_receive_from(impl, buffers, *impl.p_remote_endpoint, flags,
                              handler);
  }

  template <typename MutableBufferSequence, typename ReadHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler,
                                void(boost::system::error_code, std::size_t))
  async_receive_from(implementation_type& impl,
                     const MutableBufferSequence& buffers,
                     endpoint_type& sender_endpoint,
                     boost::asio::socket_base::message_flags flags,
                     ReadHandler&& handler) {
    boost::asio::detail::async_result_init<
        ReadHandler, void(boost::system::error_code, std::size_t)>
        init(std::forward<ReadHandler>(handler));

    if (!impl.p_next_layer_socket || !impl.p_socket_context ||
        !impl.p_local_endpoint) {
      this->get_io_service().post(
          boost::asio::detail::binder2<decltype(init.handler),
                                       boost::system::error_code, std::size_t>(
              init.handler, boost::system::error_code(
                                ssf::error::destination_address_required,
                                ssf::error::get_ssf_category()),
              0));
      return init.result.get();
    }

    if (!demultiplexer_manager_.IsBound(impl.p_next_layer_socket,
                                        impl.p_socket_context)) {
      this->get_io_service().post(
          boost::asio::detail::binder2<decltype(init.handler),
                                       boost::system::error_code, std::size_t>(
              init.handler,
              boost::system::error_code(ssf::error::network_down,
                                        ssf::error::get_ssf_category()),
              0));
      return init.result.get();
    }

    {
      std::unique_lock<std::recursive_mutex> lock(impl.p_socket_context->mutex);
      auto& op_queue = impl.p_socket_context->read_op_queue;

      typedef io::pending_read_operation<MutableBufferSequence,
                                         decltype(init.handler), protocol_type>
          op;
      typename op::ptr p = {
          boost::asio::detail::addressof(init.handler),
          boost_asio_handler_alloc_helpers::allocate(sizeof(op), init.handler),
          0};

      p.p = new (p.v) op(buffers, init.handler, &sender_endpoint);
      op_queue.push(p.p);
      p.v = p.p = 0;
    }

    demultiplexer_manager_.Read(impl.p_next_layer_socket,
                                impl.p_socket_context);

    return init.result.get();
  }

  boost::system::error_code shutdown(
      implementation_type& impl, boost::asio::socket_base::shutdown_type what,
      boost::system::error_code& ec) {
    return ec;
  }

  /// Link a demultiplexer to the next layer socket
  void StartDemultiplexer(p_next_socket_type p_socket) {
    demultiplexer_manager_.Start(p_socket);
  }

  void StopDemultiplexer(p_next_socket_type p_socket) {
    demultiplexer_manager_.Stop(p_socket);
  }

  static void StopDemultiplexer() { demultiplexer_manager_.Stop(); }

  /// Link a multiplexer to the next layer socket
  void StartMultiplexer(p_next_socket_type p_socket) {
    multiplexer_manager_.Start(p_socket);
  }

  void StopMultiplexer(p_next_socket_type p_socket) {
    multiplexer_manager_.Stop(p_socket);
  }

  static void StopMultiplexer() { multiplexer_manager_.Stop(); }

 private:
  void shutdown_service() {}

 private:
  bool ChangeBinding(implementation_type& impl, endpoint_type remote_endpoint) {
    std::unique_lock<std::recursive_mutex> lock(mutex_);

    if (!impl.p_socket_context || !impl.p_next_layer_socket ||
        !impl.p_local_endpoint || !impl.p_remote_endpoint) {
      return false;
    }

    auto pair_p_next_socket_it = next_endpoint_to_next_socket_.find(
        impl.p_next_layer_socket->local_endpoint());

    if (pair_p_next_socket_it == std::end(next_endpoint_to_next_socket_)) {
      return false;
    }

    if (demultiplexer_manager_.Unbind(impl.p_next_layer_socket,
                                      impl.p_socket_context)) {
      --pair_p_next_socket_it->second.second;
    }

    impl.p_remote_endpoint =
        std::make_shared<endpoint_type>(std::move(remote_endpoint));
    impl.p_socket_context->remote_id =
        impl.p_remote_endpoint->endpoint_context();
    impl.p_socket_context->local_id = impl.p_local_endpoint->endpoint_context();

    auto context_bound = demultiplexer_manager_.Bind(impl.p_next_layer_socket,
                                                     impl.p_socket_context);

    pair_p_next_socket_it->second.second += !!context_bound;

    return context_bound;
  }

 private:
  typedef uint32_t usage_counter_type;

  static std::recursive_mutex mutex_;
  static std::map<next_endpoint_type,
                  std::pair<p_next_socket_type, usage_counter_type>>
      next_endpoint_to_next_socket_;
  static std::set<endpoint_type> local_endpoints_;
  static multiplexing::MultiplexerManager<
      p_next_socket_type, send_datagram_type, next_endpoint_type,
      congestion_policy_type>
      multiplexer_manager_;

  static multiplexing::DemultiplexerManager<protocol_type,
                                            congestion_policy_type>
      demultiplexer_manager_;
};

template <class Protocol>
std::recursive_mutex basic_MultiplexedSocket_service<Protocol>::mutex_;

template <class Protocol>
std::map<
    typename basic_MultiplexedSocket_service<Protocol>::next_endpoint_type,
    std::pair<
        typename basic_MultiplexedSocket_service<Protocol>::p_next_socket_type,
        typename basic_MultiplexedSocket_service<Protocol>::usage_counter_type>>
    basic_MultiplexedSocket_service<Protocol>::next_endpoint_to_next_socket_;

template <class Protocol>
std::set<typename Protocol::endpoint>
    basic_MultiplexedSocket_service<Protocol>::local_endpoints_;

template <class Protocol>
multiplexing::MultiplexerManager<
    typename basic_MultiplexedSocket_service<Protocol>::p_next_socket_type,
    typename basic_MultiplexedSocket_service<Protocol>::send_datagram_type,
    typename basic_MultiplexedSocket_service<Protocol>::next_endpoint_type,
    typename basic_MultiplexedSocket_service<Protocol>::congestion_policy_type>
    basic_MultiplexedSocket_service<Protocol>::multiplexer_manager_;

template <class Protocol>
multiplexing::DemultiplexerManager<
    Protocol,
    typename basic_MultiplexedSocket_service<Protocol>::congestion_policy_type>
    basic_MultiplexedSocket_service<Protocol>::demultiplexer_manager_;

#include <boost/asio/detail/pop_options.hpp>

}  // multiplexing
}  // layer
}  // ssf

#endif  // SSF_LAYER_MULTIPLEXING_BASIC_MULTIPLEXER_SOCKET_SERVICE_H_
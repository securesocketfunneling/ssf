#ifndef SSF_CORE_VIRTUAL_ROUTING_BASIC_ROUTED_SOCKET_SERVICE_H_
#define SSF_CORE_VIRTUAL_ROUTING_BASIC_ROUTED_SOCKET_SERVICE_H_
#include <memory>

#include <boost/asio/async_result.hpp>
#include "boost/asio/detail/bind_handler.hpp"
#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/io_service.hpp>

#include <boost/system/error_code.hpp>

#include "ssf/layer/basic_impl.h"

#include "ssf/io/composed_op.h"

#include "ssf/error/error.h"
#include "ssf/io/handler_helpers.h"
#include "ssf/io/write_op.h"

#include "ssf/layer/protocol_attributes.h"

namespace ssf {
namespace layer {
namespace routing {

#include <boost/asio/detail/push_options.hpp>

template <class Protocol>
class basic_RoutedSocket_service : public boost::asio::detail::service_base<
                                       basic_RoutedSocket_service<Protocol>> {
 public:
  /// The protocol type.
  using protocol_type = Protocol;
  /// The endpoint type.
  using endpoint_type = typename protocol_type::endpoint;

  using implementation_type = basic_socket_impl<protocol_type>;
  using native_handle_type = implementation_type&;
  using native_type = native_handle_type;

 private:
  using next_endpoint_type =
      typename protocol_type::next_layer_protocol::endpoint;
  using next_socket_type = typename protocol_type::next_layer_protocol::socket;
  using endpoint_context_type = typename protocol_type::endpoint_context_type;

 public:
  explicit basic_RoutedSocket_service(boost::asio::io_service& io_service)
      : boost::asio::detail::service_base<basic_RoutedSocket_service>(
            io_service),
        p_worker_(nullptr),
        usage_count_(0),
        p_alive_(std::make_shared<bool>(true)) {}

  virtual ~basic_RoutedSocket_service() { *p_alive_ = false; }

  void construct(implementation_type& impl) {
    impl.p_socket_context =
        std::make_shared<typename protocol_type::socket_context>();
    impl.p_socket_context->p_cancelled = std::make_shared<bool>(false);
    impl.p_socket_context->p_receive_queue = nullptr;
    impl.p_socket_context->p_router = nullptr;
  }

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
    return !!impl.p_local_endpoint;
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
    if (!impl.p_local_endpoint) {
      ec.assign(ssf::error::device_or_resource_busy,
                ssf::error::get_ssf_category());
      return ec;
    }

    impl.p_socket_context->p_receive_queue = nullptr;

    std::unique_lock<std::recursive_mutex> lock(bounds_mutex_);
    bounds_.erase(*(impl.p_local_endpoint));

    return ec;
  }

  native_type native(implementation_type& impl) { return impl; }

  native_handle_type native_handle(implementation_type& impl) { return impl; }

  boost::system::error_code cancel(implementation_type& impl,
                                   boost::system::error_code& ec) {
    ec.assign(ssf::error::function_not_supported,
              ssf::error::get_ssf_category());
    return ec;
  }

  bool at_mark(const implementation_type& impl,
               boost::system::error_code& ec) const {
    ec.assign(ssf::error::function_not_supported,
              ssf::error::get_ssf_category());
    return false;
  }

  std::size_t available(const implementation_type& impl,
                        boost::system::error_code& ec) const {
    ec.assign(ssf::error::function_not_supported,
              ssf::error::get_ssf_category());
    return false;
  }

  boost::system::error_code bind(implementation_type& impl,
                                 const endpoint_type& local_endpoint,
                                 boost::system::error_code& ec) {
    std::unique_lock<std::recursive_mutex> lock(bounds_mutex_);

    if (bounds_.count(local_endpoint)) {
      ec.assign(ssf::error::device_or_resource_busy,
                ssf::error::get_ssf_category());
      return ec;
    }

    auto endpoint_context = local_endpoint.endpoint_context();
    impl.p_socket_context->network_address = endpoint_context.network_addr;

    if (endpoint_context.p_router) {
      impl.p_socket_context->p_router = endpoint_context.p_router;
    } else {
      ec.assign(ssf::error::broken_pipe, ssf::error::get_ssf_category());
      return ec;
    }

    if (!ec) {
      impl.p_socket_context->p_receive_queue =
          impl.p_socket_context->p_router->get_network_receive_queue(
              endpoint_context.network_addr, ec);
    }

    if (!ec) {
      bounds_.insert(local_endpoint);
      impl.p_local_endpoint =
          std::make_shared<endpoint_type>(std::move(local_endpoint));
    }

    return ec;
  }

  boost::system::error_code connect(implementation_type& impl,
                                    const endpoint_type& remote_endpoint,
                                    boost::system::error_code& ec) {
    ec.assign(ssf::error::function_not_supported,
              ssf::error::get_ssf_category());

    return ec;
  }

  template <typename ConnectHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(ConnectHandler, void(boost::system::error_code))
      async_connect(implementation_type& impl,
                    const endpoint_type& remote_endpoint,
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
            std::move(connect_lambda),
            init.handler));

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

    io::PostHandler(
        this->get_io_service(), init.handler,
        boost::system::error_code(ssf::error::function_not_supported,
                                  ssf::error::get_ssf_category()),
        0);

    return init.result.get();
  }

  /// Async send datagram in router
  template <typename ConstBufferSequence, typename WriteHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler,
                                void(boost::system::error_code, std::size_t))
      async_send_to(implementation_type& impl,
                    const ConstBufferSequence& buffers,
                    const endpoint_type& destination,
                    boost::asio::socket_base::message_flags flags,
                    WriteHandler&& handler) {
    boost::asio::detail::async_result_init<
        WriteHandler, void(boost::system::error_code, std::size_t)>
        init(std::forward<WriteHandler>(handler));

    if (!impl.p_socket_context->p_router) {
      io::PostHandler(this->get_io_service(), init.handler,
                      boost::system::error_code(ssf::error::no_link,
                                                ssf::error::get_ssf_category()),
                      0);

      return init.result.get();
    }

    if (boost::asio::buffer_size(buffers) > protocol_type::mtu) {
      io::PostHandler(this->get_io_service(), init.handler,
                      boost::system::error_code(ssf::error::message_size,
                                                ssf::error::get_ssf_category()),
                      0);

      return init.result.get();
    }

    register_async_op();
    impl.p_socket_context->p_router->async_send(
        protocol_type::make_datagram(buffers, *impl.p_local_endpoint,
                                     destination),
        [this, init](const boost::system::error_code& ec, std::size_t length) {
          init.handler(ec, length);
          this->unregister_async_op();
        });

    return init.result.get();
  }

  template <typename MutableBufferSequence, typename ReadHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler,
                                void(boost::system::error_code, std::size_t))
      async_receive(implementation_type& impl,
                    const MutableBufferSequence& buffers,
                    boost::asio::socket_base::message_flags flags,
                    ReadHandler&& handler) {
    boost::asio::detail::async_result_init<
        ReadHandler, void(boost::system::error_code, std::size_t)>
        init(std::forward<ReadHandler>(handler));

    io::PostHandler(
        this->get_io_service(), init.handler,
        boost::system::error_code(ssf::error::function_not_supported,
                                  ssf::error::get_ssf_category()),
        0);

    return init.result.get();
  }

  template <typename MutableBufferSequence, typename ReadHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler,
                                void(boost::system::error_code, std::size_t))
      async_receive_from(implementation_type& impl,
                         const MutableBufferSequence& buffers,
                         endpoint_type& source,
                         boost::asio::socket_base::message_flags flags,
                         ReadHandler&& handler) {
    boost::asio::detail::async_result_init<
        ReadHandler, void(boost::system::error_code, std::size_t)>
        init(std::forward<ReadHandler>(handler));

    if (!impl.p_socket_context->p_receive_queue) {
      io::PostHandler(this->get_io_service(), init.handler,
                      boost::system::error_code(ssf::error::no_link,
                                                ssf::error::get_ssf_category()),
                      0);
      return init.result.get();
    }

    register_async_op();
    impl.p_socket_context->p_receive_queue->async_get(
        [this, &impl, buffers, &source, init](
            const boost::system::error_code& ec,
            const typename protocol_type::ReceiveDatagram& datagram) {
          this->DatagramReceived(ec, datagram, impl, buffers, source,
                                 init.handler);
          this->unregister_async_op();
        });

    return init.result.get();
  }

  boost::system::error_code shutdown(
      implementation_type& impl, boost::asio::socket_base::shutdown_type what,
      boost::system::error_code& ec) {
    if (!impl.p_socket_context->p_router ||
        !impl.p_socket_context->p_receive_queue) {
      ec.assign(ssf::error::not_connected, ssf::error::get_ssf_category());

      return ec;
    }

    return ec;
  }

 private:
  void shutdown_service() {
    *p_alive_ = false;
    p_worker_.reset();
  }

  template <typename MutableBufferSequence, typename ReadHandler>
  void DatagramReceived(const boost::system::error_code& ec,
                        const typename protocol_type::ReceiveDatagram& datagram,
                        implementation_type& impl,
                        const MutableBufferSequence& buffers,
                        endpoint_type& source, ReadHandler handler) {
    if (ec) {
      handler(ec, 0);
      return;
    }

    if (datagram.header().payload_length() >
        boost::asio::buffer_size(buffers)) {
      handler(boost::system::error_code(ssf::error::message_size,
                                        ssf::error::get_ssf_category()),
              0);
      return;
    }

    auto network_source_id = datagram.header().id().left_id();

    auto& source_context = source.endpoint_context();
    source.set();
    source_context.network_addr = network_source_id;
    source_context.p_router = impl.p_local_endpoint->endpoint_context().p_router;

    auto copied =
        boost::asio::buffer_copy(buffers, datagram.payload().GetConstBuffers());

    handler(ec, copied);
    return;
  }

  void register_async_op() {
    std::unique_lock<std::recursive_mutex> lock(mutex_);
    if (usage_count_ == 0) {
      p_worker_ = std::unique_ptr<boost::asio::io_service::work>(
          new boost::asio::io_service::work(this->get_io_service()));
    }
    ++usage_count_;
  }

  void unregister_async_op() {
    std::unique_lock<std::recursive_mutex> lock(mutex_);
    --usage_count_;
    if (usage_count_ == 0) {
      p_worker_.reset();
    }
  }

 private:
  std::unique_ptr<boost::asio::io_service::work> p_worker_;
  std::recursive_mutex mutex_;
  uint64_t usage_count_;
  std::shared_ptr<bool> p_alive_;

  std::recursive_mutex bounds_mutex_;
  std::set<endpoint_type> bounds_;
};

#include <boost/asio/detail/pop_options.hpp>

}  // routing
}  // layer
}  // ssf

#endif  // SSF_CORE_VIRTUAL_ROUTING_BASIC_ROUTED_SOCKET_SERVICE_H_

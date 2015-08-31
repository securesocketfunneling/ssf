#ifndef SSF_CORE_VIRTUAL_NETWORK_NETWORK_LAYER_BASIC_NETWORK_RAW_SOCKET_SERVICE_H_
#define SSF_CORE_VIRTUAL_NETWORK_NETWORK_LAYER_BASIC_NETWORK_RAW_SOCKET_SERVICE_H_
#include <memory>

#include <boost/system/error_code.hpp>

#include <boost/asio/detail/config.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/io_service.hpp>

#include "core/virtual_network/basic_impl.h"

#include "common/io/composed_op.h"
#include "boost/asio/detail/bind_handler.hpp"

#include "common/io/handler_helpers.h"
#include "common/error/error.h"

#include "core/virtual_network/protocol_attributes.h"

namespace virtual_network {
namespace network_layer {

#include <boost/asio/detail/push_options.hpp>

template <class Protocol>
class basic_NetworkRawSocket_service
    : public boost::asio::detail::service_base<
          basic_NetworkRawSocket_service<Protocol>> {
 public:
  /// The protocol type.
  typedef Protocol protocol_type;
  /// The endpoint type.
  typedef typename protocol_type::endpoint endpoint_type;

  typedef basic_socket_impl<protocol_type> implementation_type;
  typedef implementation_type& native_handle_type;
  typedef native_handle_type native_type;

 private:
  typedef
      typename protocol_type::next_layer_protocol::endpoint next_endpoint_type;
  typedef typename protocol_type::next_layer_protocol::socket next_socket_type;
  typedef typename protocol_type::endpoint_context_type endpoint_context_type;

 public:
  explicit basic_NetworkRawSocket_service(boost::asio::io_service& io_service)
      : boost::asio::detail::service_base<basic_NetworkRawSocket_service>(
            io_service),
        p_worker_(nullptr),
        usage_count_(0),
        p_alive_(std::make_shared<bool>(true)) {}

  virtual ~basic_NetworkRawSocket_service() {}

  void construct(implementation_type& impl) {
    impl.p_socket_context =
        std::make_shared<typename protocol_type::socket_context>();
    impl.p_socket_context->p_cancelled = std::make_shared<bool>(false);
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
                                   const native_handle_type& native_socket,
                                   boost::system::error_code& ec) {
    impl = native_socket;
    return ec;
  }

  bool is_open(const implementation_type& impl) const { return true; }

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
    if (impl.p_local_endpoint) {
      boost::recursive_mutex::scoped_lock lock(protocol_type::bind_mutex_);
      protocol_type::network_to_interface_.erase(
          impl.p_local_endpoint->endpoint_context());
      protocol_type::interface_to_network_.erase(
          impl.p_local_endpoint->next_layer_endpoint());
      protocol_type::bounds_.erase(impl.p_local_endpoint->endpoint_context());
      protocol_type::used_interface_endpoints_.erase(
          impl.p_local_endpoint->next_layer_endpoint());
    }

    *(impl.p_socket_context->p_cancelled) = true;
    impl.p_next_layer_socket->cancel(ec);
    impl.p_next_layer_socket.reset();
    return ec;
  }

  native_type native(implementation_type& impl) { return impl; }

  native_handle_type native_handle(implementation_type& impl) { return impl; }

  boost::system::error_code cancel(implementation_type& impl,
                                   boost::system::error_code& ec) {
    return impl.p_next_layer_socket->cancel(ec);
  }

  bool at_mark(const implementation_type& impl,
               boost::system::error_code& ec) const {
    if (!impl.p_next_layer_socket) {
      return false;
    } else {
      return impl.p_next_layer_socket->at_mark(ec);
    }
  }

  std::size_t available(const implementation_type& impl,
                        boost::system::error_code& ec) const {
    if (!impl.p_next_layer_socket) {
      ec.assign(ssf::error::bad_file_descriptor,
                ssf::error::get_ssf_category());
      return 0;
    }

    return impl.p_next_layer_socket->available(ec);
  }

  boost::system::error_code bind(implementation_type& impl,
                                 endpoint_type local_endpoint,
                                 boost::system::error_code& ec) {
    const auto& local_endpoint_context = local_endpoint.endpoint_context();
    const auto& local_interface_endpoint = local_endpoint.next_layer_endpoint();

    boost::recursive_mutex::scoped_lock lock1(protocol_type::bind_mutex_);
    if (protocol_type::used_interface_endpoints_.count(
            local_interface_endpoint) ||
        protocol_type::bounds_.count(local_endpoint_context)) {
      ec.assign(ssf::error::address_in_use, ssf::error::get_ssf_category());
      return ec;
    }

    boost::recursive_mutex::scoped_lock lock2(
        protocol_type::next_layer_protocol::get_interface_manager().mutex);
    auto p_socket_interface_it =
        protocol_type::next_layer_protocol::get_interface_manager()
            .available_sockets.find(
                local_endpoint.next_layer_endpoint().endpoint_context());

    if (p_socket_interface_it ==
        std::end(protocol_type::next_layer_protocol::get_interface_manager()
                     .available_sockets)) {
      ec.assign(ssf::error::address_not_available,
                ssf::error::get_ssf_category());
      return ec;
    }

    protocol_type::network_to_interface_.insert(
        std::make_pair(local_endpoint_context, local_interface_endpoint));
    protocol_type::interface_to_network_.insert(
        std::make_pair(local_interface_endpoint, local_endpoint_context));
    protocol_type::bounds_.insert(std::move(local_endpoint_context));
    protocol_type::used_interface_endpoints_.insert(
        std::move(local_interface_endpoint));

    impl.p_next_layer_socket = p_socket_interface_it->second;
    impl.p_local_endpoint =
        std::make_shared<endpoint_type>(std::move(local_endpoint));

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

    if (!impl.p_next_layer_socket || !impl.p_local_endpoint) {
      io::PostHandler(this->get_io_service(), init.handler,
                      boost::system::error_code(ssf::error::identifier_removed,
                                                ssf::error::get_ssf_category()),
                      0);

      return init.result.get();
    }

    register_async_op();
    auto p_socket_cancelled = impl.p_socket_context->p_cancelled;
    auto p_alive = p_alive_;
    auto user_handler = [this, p_alive, p_socket_cancelled, init](
        const boost::system::error_code& ec, std::size_t length) {
      if (!(*p_alive)) {
        return;
      }
      if (!*p_socket_cancelled) {
        init.handler(ec, length);
      }

      this->unregister_async_op();
    };

    impl.p_next_layer_socket->async_send(buffers, std::move(user_handler));

    return init.result.get();
  }

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
      async_receive(implementation_type& impl,
                    const MutableBufferSequence& buffers,
                    boost::asio::socket_base::message_flags flags,
                    ReadHandler&& handler) {
    boost::asio::detail::async_result_init<
        ReadHandler, void(boost::system::error_code, std::size_t)>
        init(std::forward<ReadHandler>(handler));

    register_async_op();
    auto p_socket_cancelled = impl.p_socket_context->p_cancelled;
    auto p_alive = p_alive_;
    auto user_handler = [this, buffers, p_alive, p_socket_cancelled, init](
        const boost::system::error_code& ec, std::size_t length) {
      if (!(*p_alive)) {
        return;
      }

      if (!(*p_socket_cancelled)) {
        init.handler(ec, length);
      }

      this->unregister_async_op();
    };

    impl.p_next_layer_socket->async_receive(buffers, std::move(user_handler));

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

    io::PostHandler(
        this->get_io_service(), init.handler,
        boost::system::error_code(ssf::error::function_not_supported,
                                  ssf::error::get_ssf_category()),
        0);

    return init.result.get();
  }

  boost::system::error_code shutdown(
      implementation_type& impl, boost::asio::socket_base::shutdown_type what,
      boost::system::error_code& ec) {
    if (!impl.p_next_layer_socket) {
      ec.assign(ssf::error::not_connected, ssf::error::get_ssf_category());

      return ec;
    }

    impl.p_next_layer_socket->shutdown(what, ec);

    return ec;
  }

 private:
  void shutdown_service() {
    *p_alive_ = false;
    p_worker_.reset();
  }

  void register_async_op() {
    boost::recursive_mutex::scoped_lock lock(mutex_);
    if (usage_count_ == 0) {
      // p_worker_ =
      //     std::make_unique<boost::asio::io_service::work>(this->get_io_service());

      p_worker_ = std::unique_ptr<boost::asio::io_service::work>(
          new boost::asio::io_service::work(this->get_io_service()));
    }
    ++usage_count_;
  }

  void unregister_async_op() {
    boost::recursive_mutex::scoped_lock lock(mutex_);
    --usage_count_;
    if (usage_count_ == 0) {
      p_worker_.reset();
    }
  }

 private:
  std::unique_ptr<boost::asio::io_service::work> p_worker_;
  uint64_t usage_count_;
  std::shared_ptr<bool> p_alive_;
  boost::recursive_mutex mutex_;
};

#include <boost/asio/detail/pop_options.hpp>

}  // network_layer
}  // virtual_network

#endif  // SSF_CORE_VIRTUAL_NETWORK_NETWORK_LAYER_BASIC_NETWORK_RAW_SOCKET_SERVICE_H_

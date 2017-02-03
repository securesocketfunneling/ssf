#ifndef SSF_LAYER_CRYPTOGRAPHY_BASIC_CRYPTO_STREAM_H_
#define SSF_LAYER_CRYPTOGRAPHY_BASIC_CRYPTO_STREAM_H_

#include <map>
#include <memory>
#include <string>

#include <boost/asio/async_result.hpp>
#include <boost/asio/basic_stream_socket.hpp>
#include <boost/asio/detail/config.hpp>
#include <boost/asio/handler_type.hpp>
#include <boost/asio/io_service.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/system/error_code.hpp>

#include "ssf/io/handler_helpers.h"
#include "ssf/error/error.h"

#include "ssf/layer/basic_endpoint.h"
#include "ssf/layer/basic_impl.h"
#include "ssf/layer/basic_resolver.h"

#include "ssf/layer/cryptography/crypto_stream_op.h"

#include "ssf/layer/parameters.h"
#include "ssf/layer/protocol_attributes.h"

namespace ssf {
namespace layer {
namespace cryptography {

template <class Protocol, class CryptoProtocol>
class basic_CryptoStreamSocket_service;
template <class Protocol, class CryptoProtocol>
class basic_CryptoStreamAcceptor_service;

template <class NextLayer, template <class> class Crypto>
class basic_CryptoStreamProtocol {
 public:
  using CryptoProtocol = Crypto<NextLayer>;

  enum {
    id = CryptoProtocol::id,
    overhead = CryptoProtocol::overhead,
    facilities = ssf::layer::facilities::stream,
    mtu = CryptoProtocol::mtu
  };
  enum { endpoint_stack_size = CryptoProtocol::endpoint_stack_size };

  typedef NextLayer next_layer_protocol;
  typedef char socket_context;
  typedef char acceptor_context;
  using endpoint_context_type = typename CryptoProtocol::endpoint_context_type;
  using next_endpoint_type = typename next_layer_protocol::endpoint;

  typedef basic_VirtualLink_endpoint<basic_CryptoStreamProtocol> endpoint;
  typedef basic_VirtualLink_resolver<basic_CryptoStreamProtocol> resolver;
  typedef boost::asio::basic_stream_socket<
      basic_CryptoStreamProtocol,
      basic_CryptoStreamSocket_service<basic_CryptoStreamProtocol,
                                       CryptoProtocol>> socket;
  typedef boost::asio::basic_socket_acceptor<
      basic_CryptoStreamProtocol,
      basic_CryptoStreamAcceptor_service<basic_CryptoStreamProtocol,
                                         CryptoProtocol>> acceptor;

 private:
  using query = typename resolver::query;

 public:
  static std::string get_name() {
    std::string name =
        CryptoProtocol::get_name() + "_" + next_layer_protocol::get_name();
    return name;
  }

  static endpoint make_endpoint(boost::asio::io_service& io_service,
                                typename query::const_iterator parameters_it,
                                uint32_t, boost::system::error_code& ec) {
    auto endpoint_context = CryptoProtocol::make_endpoint_context(
        io_service, parameters_it, id, ec);
    if (ec) {
      return endpoint();
    }

    auto next_endpoint =
        next_layer_protocol::make_endpoint(io_service, ++parameters_it, id, ec);

    return endpoint(std::move(endpoint_context), std::move(next_endpoint));
  }

  static void add_params_from_property_tree(
      query* p_query, const boost::property_tree::ptree& property_tree,
      bool connect, boost::system::error_code& ec) {
    auto sublayer = property_tree.get_child_optional("sublayer");
    if (!sublayer) {
      ec.assign(ssf::error::missing_config_parameters,
                ssf::error::get_ssf_category());
      return;
    }

    CryptoProtocol::add_params_from_property_tree(p_query, property_tree,
                                                  connect, ec);
    next_layer_protocol::add_params_from_property_tree(p_query, *sublayer,
                                                       connect, ec);
  }
};

#include <boost/asio/detail/push_options.hpp>

template <class Protocol, class CryptoProtocol>
class basic_CryptoStreamSocket_service
    : public boost::asio::detail::service_base<
          basic_CryptoStreamSocket_service<Protocol, CryptoProtocol>> {
 public:
  using protocol_type = Protocol;

  using crypto_stream_type = typename CryptoProtocol::Stream;

  using socket_context = typename protocol_type::socket_context;
  using endpoint_type = typename protocol_type::endpoint;

  using implementation_type =
      basic_socket_impl_ex<socket_context, endpoint_type, crypto_stream_type>;

  using native_handle_type = implementation_type&;
  using native_type = native_handle_type;

 public:
  explicit basic_CryptoStreamSocket_service(boost::asio::io_service& io_service)
      : boost::asio::detail::service_base<basic_CryptoStreamSocket_service>(
            io_service) {}

  virtual ~basic_CryptoStreamSocket_service() {}

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
    if (!impl.p_next_layer_socket) {
      return false;
    }

    return impl.p_next_layer_socket->next_layer().is_open();
  }

  endpoint_type remote_endpoint(const implementation_type& impl,
                                boost::system::error_code& ec) const {
    if (impl.p_remote_endpoint) {
      ec.assign(ssf::error::success, ssf::error::get_ssf_category());
      return *impl.p_remote_endpoint;
    } else {
      ec.assign(ssf::error::no_link, ssf::error::get_ssf_category());
      return endpoint_type();
    }
  }

  endpoint_type local_endpoint(const implementation_type& impl,
                               boost::system::error_code& ec) const {
    if (impl.p_local_endpoint) {
      ec.assign(ssf::error::success, ssf::error::get_ssf_category());
      return *impl.p_local_endpoint;
    } else {
      ec.assign(ssf::error::no_link, ssf::error::get_ssf_category());
      return endpoint_type();
    }
  }

  boost::system::error_code close(implementation_type& impl,
                                  boost::system::error_code& ec) {
    if (!impl.p_next_layer_socket) {
      ec.assign(ssf::error::broken_pipe, ssf::error::get_ssf_category());
      return ec;
    }

    impl.p_next_layer_socket->shutdown(boost::asio::socket_base::shutdown_both,
                                       ec);
    return impl.p_next_layer_socket->next_layer().close(ec);
  }

  native_type native(implementation_type& impl) { return impl; }

  native_handle_type native_handle(implementation_type& impl) { return impl; }

  boost::system::error_code cancel(implementation_type& impl,
                                   boost::system::error_code& ec) {
    if (!impl.p_next_layer_socket) {
      return ec;
    }

    return impl.p_next_layer_socket->next_layer().cancel(ec);
  }

  bool at_mark(const implementation_type& impl,
               boost::system::error_code& ec) const {
    if (!impl.p_next_layer_socket) {
      return false;
    }

    return impl.p_next_layer_socket->next_layer().at_mark(ec);
  }

  std::size_t available(const implementation_type& impl,
                        boost::system::error_code& ec) const {
    if (!impl.p_next_layer_socket) {
      ec.assign(ssf::error::bad_file_descriptor,
                ssf::error::get_ssf_category());
      return 0;
    }

    return impl.p_next_layer_socket->next_layer().available(ec);
  }

  boost::system::error_code bind(implementation_type& impl,
                                 const endpoint_type& endpoint,
                                 boost::system::error_code& ec) {
    impl.p_next_layer_socket = std::make_shared<crypto_stream_type>(
        this->get_io_service(), endpoint.endpoint_context());

    impl.p_local_endpoint = std::make_shared<endpoint_type>(endpoint);

    auto result = impl.p_next_layer_socket->next_layer().open(
        protocol_type::next_layer_protocol(), ec);

    if (!ec) {
      result = impl.p_next_layer_socket->next_layer().bind(
          endpoint.next_layer_endpoint(), ec);
    }

    return result;
  }

  /// Connect the layer behind the ssl stream
  /// Then handshake
  boost::system::error_code connect(implementation_type& impl,
                                    const endpoint_type& peer_endpoint,
                                    boost::system::error_code& ec) {
    impl.p_next_layer_socket = std::make_shared<crypto_stream_type>(
        this->get_io_service(), peer_endpoint.endpoint_context());

    impl.p_remote_endpoint = std::make_shared<endpoint_type>(peer_endpoint);

    impl.p_next_layer_socket->next_layer().connect(
        peer_endpoint.next_layer_endpoint(), ec);

    if (!ec) {
      impl.p_local_endpoint = std::make_shared<endpoint_type>(
          peer_endpoint.endpoint_context(),
          impl.p_next_layer_socket->next_layer().local_endpoint(ec));
    }

    if (!ec) {
      impl.p_next_layer_socket->handshake(
          CryptoProtocol::handshake_type::client, ec);
    }

    return ec;
  }

  /// Async connect the layer behind the ssl stream
  /// Then async handshake, then async execute handler
  template <typename ConnectHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(ConnectHandler, void(boost::system::error_code))
      async_connect(implementation_type& impl,
                    const endpoint_type& peer_endpoint,
                    ConnectHandler&& handler) {
    boost::asio::detail::async_result_init<ConnectHandler,
                                           void(boost::system::error_code)>
        init(std::forward<ConnectHandler>(handler));

    impl.p_next_layer_socket = std::make_shared<crypto_stream_type>(
        this->get_io_service(), peer_endpoint.endpoint_context());

    impl.p_remote_endpoint = std::make_shared<endpoint_type>(peer_endpoint);
    impl.p_local_endpoint =
        std::make_shared<endpoint_type>(peer_endpoint.endpoint_context());

    detail::CryptoStreamConnectOp<
        CryptoProtocol, crypto_stream_type, endpoint_type,
          decltype(init.handler)> (
        *impl.p_next_layer_socket, impl.p_local_endpoint.get(), peer_endpoint,
        init.handler)();

    return init.result.get();
  }

  template <typename ConstBufferSequence>
  std::size_t send(implementation_type& impl,
                   const ConstBufferSequence& buffers,
                   boost::asio::socket_base::message_flags flags,
                   boost::system::error_code& ec) {
    if (!impl.p_next_layer_socket) {
      ec.assign(ssf::error::broken_pipe, ssf::error::get_ssf_category());
      return 0;
    }

    return impl.p_next_layer_socket->write_some(buffers, ec);
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

    if (!impl.p_next_layer_socket) {
      io::PostHandler(this->get_io_service(), init.handler,
                      boost::system::error_code(ssf::error::broken_pipe,
                                                ssf::error::get_ssf_category()),
                      0);
      return init.result.get();
    }

    impl.p_next_layer_socket->async_write_some(buffers, init.handler);

    return init.result.get();
  }

  template <typename MutableBufferSequence>
  std::size_t receive(implementation_type& impl,
                      const MutableBufferSequence& buffers,
                      boost::asio::socket_base::message_flags flags,
                      boost::system::error_code& ec) {
    if (!impl.p_next_layer_socket) {
      ec.assign(ssf::error::broken_pipe, ssf::error::get_ssf_category());
      return 0;
    }

    return impl.p_next_layer_socket->read_some(buffers, ec);
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

    if (!impl.p_next_layer_socket) {
      io::PostHandler(this->get_io_service(), init.handler,
                      boost::system::error_code(ssf::error::broken_pipe,
                                                ssf::error::get_ssf_category()),
                      0);
      return init.result.get();
    }

    impl.p_next_layer_socket->async_read_some(buffers, init.handler);

    return init.result.get();
  }

  boost::system::error_code shutdown(
      implementation_type& impl, boost::asio::socket_base::shutdown_type what,
      boost::system::error_code& ec) {
    if (!impl.p_next_layer_socket) {
      ec.assign(ssf::error::broken_pipe, ssf::error::get_ssf_category());
      return ec;
    }

    return impl.p_next_layer_socket->next_layer().shutdown(what, ec);
  }

 private:
  void shutdown_service() {}
};

template <class Protocol, class CryptoProtocol>
class basic_CryptoStreamAcceptor_service
    : public boost::asio::detail::service_base<
          basic_CryptoStreamAcceptor_service<Protocol, CryptoProtocol>> {
 public:
  using protocol_type = Protocol;

  using endpoint_type = typename protocol_type::endpoint;

  using implementation_type = basic_acceptor_impl<protocol_type>;
  using native_handle_type = implementation_type&;
  using native_type = native_handle_type;

 private:
  using next_acceptor_type =
      typename protocol_type::next_layer_protocol::acceptor;
  using crypto_stream_type = typename CryptoProtocol::Stream;

 public:
  explicit basic_CryptoStreamAcceptor_service(
      boost::asio::io_service& io_service)
      : boost::asio::detail::service_base<basic_CryptoStreamAcceptor_service>(
            io_service) {}

  virtual ~basic_CryptoStreamAcceptor_service() {}

  void construct(implementation_type& impl) {
    impl.p_next_layer_acceptor =
        std::make_shared<next_acceptor_type>(this->get_io_service());
  }

  void destroy(implementation_type& impl) {
    impl.p_acceptor_context.reset();
    impl.p_local_endpoint.reset();
    impl.p_remote_endpoint.reset();
    impl.p_next_layer_acceptor.reset();
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
    return impl.p_next_layer_acceptor->open(
        typename protocol_type::next_layer_protocol(), ec);
  }

  boost::system::error_code assign(implementation_type& impl,
                                   const protocol_type& protocol,
                                   native_handle_type& native_socket,
                                   boost::system::error_code& ec) {
    impl = native_socket;
    return ec;
  }

  bool is_open(const implementation_type& impl) const {
    return impl.p_next_layer_acceptor->is_open();
  }

  endpoint_type local_endpoint(const implementation_type& impl,
                               boost::system::error_code& ec) const {
    if (impl.p_local_endpoint) {
      ec.assign(ssf::error::success, ssf::error::get_ssf_category());
      return *impl.p_local_endpoint;
    } else {
      ec.assign(ssf::error::no_link, ssf::error::get_ssf_category());
      return endpoint_type();
    }
  }

  boost::system::error_code close(implementation_type& impl,
                                  boost::system::error_code& ec) {
    return impl.p_next_layer_acceptor->close(ec);
  }

  native_type native(implementation_type& impl) { return impl; }

  native_handle_type native_handle(implementation_type& impl) { return impl; }

  /// Set a socket option.
  template <typename SettableSocketOption>
  boost::system::error_code set_option(implementation_type& impl,
      const SettableSocketOption& option, boost::system::error_code& ec)
  {
    if (impl.p_next_layer_acceptor) {
      return impl.p_next_layer_acceptor->set_option(option, ec);
    }
    return ec;
  }

  boost::system::error_code cancel(implementation_type& impl,
                                   boost::system::error_code& ec) {
    return impl.p_next_layer_acceptor->cancel(ec);
  }

  boost::system::error_code bind(implementation_type& impl,
                                 const endpoint_type& endpoint,
                                 boost::system::error_code& ec) {
    impl.p_local_endpoint = std::make_shared<endpoint_type>(endpoint);
    return impl.p_next_layer_acceptor->bind(endpoint.next_layer_endpoint(), ec);
  }

  boost::system::error_code listen(implementation_type& impl, int backlog,
                                   boost::system::error_code& ec) {
    return impl.p_next_layer_acceptor->listen(backlog, ec);
  }

  template <typename Protocol1, typename SocketService>
  boost::system::error_code accept(
      implementation_type& impl,
      boost::asio::basic_socket<Protocol1, SocketService>& peer,
      endpoint_type* p_peer_endpoint, boost::system::error_code& ec,
      typename std::enable_if<boost::thread_detail::is_convertible<
          protocol_type, Protocol1>::value>::type* = 0) {
    auto& peer_impl = peer.native_handle();
    peer_impl.p_next_layer_socket = std::make_shared<crypto_stream_type>(
        this->get_io_service(), impl.p_local_endpoint->endpoint_context());
    peer_impl.p_local_endpoint = impl.p_local_endpoint;
    peer_impl.p_remote_endpoint =
        std::make_shared<typename Protocol1::endpoint>(
            impl.p_local_endpoint->endpoint_context());

    impl.p_next_layer_acceptor->accept(
        peer_impl.p_next_layer_socket->next_layer(),
        peer_impl.p_remote_endpoint->next_layer_endpoint(), ec);

    if (!ec) {
      peer_impl.p_remote_endpoint->set();

      if (p_peer_endpoint) {
        *p_peer_endpoint = *(peer_impl.p_remote_endpoint);
      }

      peer_impl.p_next_layer_socket->handshake(
          CryptoProtocol::handshake_type::server, ec);
    }

    return ec;
  }

  template <typename Protocol1, typename SocketService, typename AcceptHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(AcceptHandler, void(boost::system::error_code))
      async_accept(implementation_type& impl,
                   boost::asio::basic_socket<Protocol1, SocketService>& peer,
                   endpoint_type* p_peer_endpoint, AcceptHandler&& handler,
                   typename std::enable_if<boost::thread_detail::is_convertible<
                       protocol_type, Protocol1>::value>::type* = 0) {
    boost::asio::detail::async_result_init<AcceptHandler,
                                           void(boost::system::error_code)>
        init(std::forward<AcceptHandler>(handler));

    auto& peer_impl = peer.native_handle();
    peer_impl.p_next_layer_socket = std::make_shared<crypto_stream_type>(
        this->get_io_service(), impl.p_local_endpoint->endpoint_context());
    peer_impl.p_local_endpoint = std::make_shared<typename Protocol1::endpoint>(
        impl.p_local_endpoint->endpoint_context());
    peer_impl.p_remote_endpoint =
        std::make_shared<typename Protocol1::endpoint>(
            impl.p_local_endpoint->endpoint_context());

    detail::CryptoStreamAcceptOp<
        CryptoProtocol, next_acceptor_type,
        typename std::remove_reference<typename boost::asio::basic_socket<
            Protocol1, SocketService>::native_handle_type>::type,
        endpoint_type,
        decltype(init.handler)> (
        *impl.p_next_layer_acceptor, &peer_impl, p_peer_endpoint,
        init.handler)();

    return init.result.get();
  }

 private:
  void shutdown_service() {}
};

#include <boost/asio/detail/pop_options.hpp>

}  // cryptography
}  // layer
}  // ssf

#endif  // SSF_LAYER_CRYPTOGRAPHY_BASIC_CRYPTO_STREAM_H_

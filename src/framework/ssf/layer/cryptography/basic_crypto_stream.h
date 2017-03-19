#ifndef SSF_LAYER_CRYPTOGRAPHY_BASIC_CRYPTO_STREAM_H_
#define SSF_LAYER_CRYPTOGRAPHY_BASIC_CRYPTO_STREAM_H_

#include <map>
#include <memory>
#include <queue>
#include <string>

#include <boost/asio/async_result.hpp>
#include <boost/asio/basic_stream_socket.hpp>
#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/op_queue.hpp>
#include <boost/asio/handler_type.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/thread.hpp>
#include <boost/system/error_code.hpp>

#include "ssf/io/handler_helpers.h"
#include "ssf/error/error.h"

#include "ssf/layer/basic_endpoint.h"
#include "ssf/layer/basic_impl.h"
#include "ssf/layer/basic_resolver.h"

#include "ssf/layer/cryptography/crypto_stream_op.h"

#include "ssf/layer/parameters.h"
#include "ssf/layer/protocol_attributes.h"

#include "ssf/io/accept_op.h"

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

  struct acceptor_context {
    using op_queue_type =
        boost::asio::detail::op_queue<io::basic_pending_accept_operation<
            basic_CryptoStreamProtocol<NextLayer, Crypto>>>;
    using connection_queue_type = std::queue<std::shared_ptr<socket>>;

    boost::recursive_mutex accept_mutex;
    bool listening;
    uint64_t max_connections_count;
    op_queue_type accept_queue;
    connection_queue_type connection_queue;
  };

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

    detail::CryptoStreamConnectOp<CryptoProtocol, crypto_stream_type,
                                  endpoint_type, decltype(init.handler)> (
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
  using acceptor_type = typename protocol_type::acceptor;
  using socket_type = typename protocol_type::socket;
  using endpoint_type = typename protocol_type::endpoint;
  using acceptor_context_type = typename protocol_type::acceptor_context;
  using next_acceptor_type =
      typename protocol_type::next_layer_protocol::acceptor;
  using next_socket_type = typename protocol_type::next_layer_protocol::socket;
  using next_endpoint_type =
      typename protocol_type::next_layer_protocol::endpoint;

  using p_socket_type = std::shared_ptr<socket_type>;
  using p_endpoint_type = std::shared_ptr<endpoint_type>;
  using p_acceptor_type = std::shared_ptr<acceptor_type>;
  using p_acceptor_context_type = std::shared_ptr<acceptor_context_type>;
  using p_next_acceptor_type = std::shared_ptr<next_acceptor_type>;
  using p_next_socket_type = std::shared_ptr<next_socket_type>;
  using p_next_endpoint_type = std::shared_ptr<next_endpoint_type>;

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
    impl.p_acceptor_context = std::make_shared<acceptor_context_type>();
    impl.p_acceptor_context->listening = false;
    impl.p_acceptor_context->max_connections_count = 0;
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
    boost::system::error_code close_ec(ssf::error::interrupted,
                                       ssf::error::get_ssf_category());

    {
      boost::recursive_mutex::scoped_lock lock(
          impl.p_acceptor_context->accept_mutex);
      impl.p_acceptor_context->max_connections_count = 0;
      impl.p_acceptor_context->listening = false;
    }

    impl.p_next_layer_acceptor->close(ec);

    clean_pending_accepts(impl.p_acceptor_context, close_ec);
    connection_queue_handler(impl.p_acceptor_context, close_ec);

    return ec;
  }

  native_type native(implementation_type& impl) { return impl; }

  native_handle_type native_handle(implementation_type& impl) { return impl; }

  /// Set a socket option.
  template <typename SettableSocketOption>
  boost::system::error_code set_option(implementation_type& impl,
                                       const SettableSocketOption& option,
                                       boost::system::error_code& ec) {
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
    impl.p_next_layer_acceptor->listen(backlog, ec);
    if (ec) {
      return ec;
    }

    {
      boost::recursive_mutex::scoped_lock lock(
          impl.p_acceptor_context->accept_mutex);
      impl.p_acceptor_context->max_connections_count =
          static_cast<uint64_t>(backlog);
      impl.p_acceptor_context->listening = true;
    }

    start_accepting(impl.p_next_layer_acceptor, impl.p_local_endpoint,
                    impl.p_acceptor_context);
    return ec;
  }

  template <typename Protocol1, typename SocketService>
  boost::system::error_code accept(
      implementation_type& impl,
      boost::asio::basic_socket<Protocol1, SocketService>& peer,
      endpoint_type* p_peer_endpoint, boost::system::error_code& ec,
      typename std::enable_if<boost::thread_detail::is_convertible<
          protocol_type, Protocol1>::value>::type* = 0) {
    auto p_acceptor_context = impl.p_acceptor_context;
    auto& connection_queue = p_acceptor_context->connection_queue;

    while (true) {
      boost::this_thread::sleep(boost::posix_time::milliseconds(100));
      boost::recursive_mutex::scoped_lock lock(
          p_acceptor_context->accept_mutex);
      if (connection_queue.empty()) {
        continue;
      }

      auto p_socket = connection_queue.front();
      connection_queue.pop();

      auto& peer_impl = peer.native_handle();
      peer_impl = p_socket->native_handle();
      break;
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
    peer_impl.p_local_endpoint = std::make_shared<typename Protocol1::endpoint>(
        impl.p_local_endpoint->endpoint_context());
    peer_impl.p_remote_endpoint =
        std::make_shared<typename Protocol1::endpoint>(
            impl.p_local_endpoint->endpoint_context());

    auto p_acceptor_context = impl.p_acceptor_context;

    {
      boost::recursive_mutex::scoped_lock lock(
          p_acceptor_context->accept_mutex);
      auto& accept_queue = p_acceptor_context->accept_queue;

      typedef io::pending_accept_operation<
          typename boost::asio::handler_type<
              AcceptHandler, void(boost::system::error_code)>::type,
          protocol_type> op;
      typename op::ptr p = {
          boost::asio::detail::addressof(init.handler),
          boost_asio_handler_alloc_helpers::allocate(sizeof(op), init.handler),
          0};

      p.p = new (p.v) op(peer, peer_impl.p_remote_endpoint.get(), init.handler);
      accept_queue.push(p.p);
      p.v = p.p = 0;
    }

    connection_queue_handler(impl.p_acceptor_context);

    return init.result.get();
  }

 private:
  void shutdown_service() {}

  void start_accepting(p_next_acceptor_type p_next_layer_acceptor,
                       p_endpoint_type p_local_endpoint,
                       p_acceptor_context_type p_acceptor_context) {
    if (!p_next_layer_acceptor->is_open()) {
      return;
    }

    {
      boost::recursive_mutex::scoped_lock lock(
          p_acceptor_context->accept_mutex);
      if (!p_acceptor_context->listening) {
        return;
      }
    }

    auto p_socket = std::make_shared<socket_type>(this->get_io_service());

    auto& peer_impl = p_socket->native_handle();
    peer_impl.p_next_layer_socket = std::make_shared<crypto_stream_type>(
        this->get_io_service(), p_local_endpoint->endpoint_context());
    peer_impl.p_local_endpoint =
        std::make_shared<endpoint_type>(p_local_endpoint->endpoint_context());
    peer_impl.p_remote_endpoint =
        std::make_shared<endpoint_type>(p_local_endpoint->endpoint_context());

    p_next_layer_acceptor->async_accept(
        peer_impl.p_next_layer_socket->next_layer(),
        peer_impl.p_remote_endpoint->next_layer_endpoint(),
        boost::bind(&basic_CryptoStreamAcceptor_service::accepted, this,
                    p_next_layer_acceptor, p_local_endpoint, p_acceptor_context,
                    p_socket, _1));
  }

  void accepted(p_next_acceptor_type p_next_layer_acceptor,
                p_endpoint_type p_local_endpoint,
                p_acceptor_context_type p_acceptor_context,
                p_socket_type p_socket, const boost::system::error_code& ec) {
    // Do not break accept loop
    start_accepting(p_next_layer_acceptor, p_local_endpoint,
                    p_acceptor_context);

    auto& peer_impl = p_socket->native_handle();
    if (ec) {
      SSF_LOG(kLogDebug) << "network[crypto]: could not accept connection ("
                         << ec.message() << ")";
      boost::system::error_code close_ec;

      peer_impl.p_next_layer_socket->close(close_ec);
      return;
    }

    peer_impl.p_remote_endpoint->set();

    peer_impl.p_local_endpoint->next_layer_endpoint() =
        peer_impl.p_next_layer_socket->next_layer().local_endpoint();
    peer_impl.p_local_endpoint->set();

    peer_impl.p_next_layer_socket->async_handshake(
        CryptoProtocol::handshake_type::server,
        boost::bind(&basic_CryptoStreamAcceptor_service::crypto_handshaked,
                    this, p_acceptor_context, p_socket, _1));
  }

  void crypto_handshaked(p_acceptor_context_type p_acceptor_context,
                         p_socket_type p_socket,
                         const boost::system::error_code& ec) {
    if (ec) {
      boost::system::error_code close_ec;
      p_socket->shutdown(boost::asio::socket_base::shutdown_both, close_ec);
      p_socket->close(close_ec);
      return;
    }

    {
      boost::recursive_mutex::scoped_lock lock(
          p_acceptor_context->accept_mutex);
      auto& connection_queue = p_acceptor_context->connection_queue;
      if (connection_queue.size() < p_acceptor_context->max_connections_count) {
        // save connection for further accept operations
        connection_queue.emplace(p_socket);
      } else {
        // drop connection
        boost::system::error_code close_ec;
        p_socket->shutdown(boost::asio::socket_base::shutdown_both, close_ec);
        p_socket->close(close_ec);
      }
    }

    connection_queue_handler(p_acceptor_context);
  }

  void connection_queue_handler(
      p_acceptor_context_type p_acceptor_context,
      const boost::system::error_code& ec = boost::system::error_code()) {
    boost::recursive_mutex::scoped_lock lock(p_acceptor_context->accept_mutex);

    auto& connection_queue = p_acceptor_context->connection_queue;
    auto& accept_queue = p_acceptor_context->accept_queue;

    if (ec) {
      // clean connection queue
      while (!connection_queue.empty()) {
        auto p_socket = connection_queue.front();
        boost::system::error_code close_ec;
        p_socket->shutdown(boost::asio::socket_base::shutdown_both, close_ec);
        p_socket->close(close_ec);
        connection_queue.pop();
      }
      return;
    }

    if (!connection_queue.empty() && !accept_queue.empty()) {
      auto connection = connection_queue.front();
      connection_queue.pop();
      auto* p_accept_op = accept_queue.front();
      accept_queue.pop();

      boost::system::error_code remote_ep_ec;
      p_accept_op->set_p_endpoint(connection->remote_endpoint(remote_ep_ec));

      auto& peer = p_accept_op->peer();

      auto& native_handle = peer.native_handle();
      native_handle = connection->native_handle();

      auto do_complete = [p_accept_op, ec]() { p_accept_op->complete(ec); };
      this->get_io_service().post(do_complete);

      this->get_io_service().post(boost::bind(
          &basic_CryptoStreamAcceptor_service::connection_queue_handler, this,
          p_acceptor_context, ec));
    }
  }

  void clean_pending_accepts(p_acceptor_context_type p_acceptor_context,
                             const boost::system::error_code& ec) {
    boost::recursive_mutex::scoped_lock(p_acceptor_context->accept_mutex);
    auto& accept_queue = p_acceptor_context->accept_queue;

    while (!accept_queue.empty()) {
      auto* p_accept_op = accept_queue.front();
      accept_queue.pop();
      p_accept_op->complete(ec);
    }
  }
};

#include <boost/asio/detail/pop_options.hpp>

}  // cryptography
}  // layer
}  // ssf

#endif  // SSF_LAYER_CRYPTOGRAPHY_BASIC_CRYPTO_STREAM_H_

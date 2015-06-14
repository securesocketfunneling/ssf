#ifndef SSF_CORE_VIRTUAL_NETWORK_BASIC_EMPTY_STREAM_H_
#define SSF_CORE_VIRTUAL_NETWORK_BASIC_EMPTY_STREAM_H_

#include <memory>
#include <string>
#include <map>
#include <type_traits>

#include <boost/system/error_code.hpp>

#include <boost/asio/detail/config.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/basic_stream_socket.hpp>
#include <boost/asio/io_service.hpp>

#include <boost/system/error_code.hpp>

#include <boost/thread.hpp>

#include "common/error/error.h"

#include "core/virtual_network/basic_impl.h"
#include "core/virtual_network/basic_resolver.h"
#include "core/virtual_network/basic_endpoint.h"

#include "core/virtual_network/protocol_attributes.h"

#include "common/io/composed_op.h"
#include "core/virtual_network/accept_op.h"

namespace virtual_network {

template <class Protocol>
class VirtualEmptyStreamSocket_service;
template <class Protocol>
class VirtualEmptyStreamAcceptor_service;

template <class NextLayer>
class VirtualEmptyStreamProtocol {
 public:
  enum {
    id = 3,
    overhead = 0,
    facilities = virtual_network::facilities::stream,
    mtu = NextLayer::mtu - overhead
  };
  enum { endpoint_stack_size = NextLayer::endpoint_stack_size };

  typedef NextLayer next_layer_protocol;
  typedef int socket_context;
  typedef int acceptor_context;
  typedef int endpoint_context_type;
  typedef basic_VirtualLink_endpoint<VirtualEmptyStreamProtocol> endpoint;
  typedef basic_VirtualLink_resolver<VirtualEmptyStreamProtocol> resolver;
  typedef boost::asio::basic_stream_socket<
      VirtualEmptyStreamProtocol,
      VirtualEmptyStreamSocket_service<VirtualEmptyStreamProtocol>> socket;
  typedef boost::asio::basic_socket_acceptor<
      VirtualEmptyStreamProtocol,
      VirtualEmptyStreamAcceptor_service<VirtualEmptyStreamProtocol>> acceptor;

  typedef std::map<std::string, std::string> raw_endpoint_parameters;

 public:
  template <class Container>
  static endpoint make_endpoint(
      boost::asio::io_service& io_service,
      typename Container::const_iterator parameters_it, uint32_t,
      boost::system::error_code& ec) {
    return endpoint(next_layer_protocol::template make_endpoint<Container>(
        io_service, parameters_it, id, ec));
  }
};

#include <boost/asio/detail/push_options.hpp>

template <class Protocol>
class VirtualEmptyStreamSocket_service
    : public boost::asio::detail::service_base<
          VirtualEmptyStreamSocket_service<Protocol>> {
 public:
  /// The protocol type.
  typedef Protocol protocol_type;
  /// The endpoint type.
  typedef typename protocol_type::endpoint endpoint_type;

  typedef basic_socket_impl<protocol_type> implementation_type;
  typedef implementation_type& native_handle_type;
  typedef native_handle_type native_type;

 private:
  typedef typename protocol_type::next_layer_protocol::socket next_socket_type;

 public:
  explicit VirtualEmptyStreamSocket_service(boost::asio::io_service& io_service)
      : boost::asio::detail::service_base<VirtualEmptyStreamSocket_service>(
            io_service) {}

  virtual ~VirtualEmptyStreamSocket_service() {}

  void construct(implementation_type& impl) {
    impl.p_next_layer_socket =
        std::make_shared<next_socket_type>(this->get_io_service());
  }

  void destroy(implementation_type& impl) {
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
    return impl.p_next_layer_socket->open(typename protocol_type::next_layer_protocol(),
                                          ec);
  }

  boost::system::error_code assign(implementation_type& impl,
                                   const protocol_type& protocol,
                                   const native_handle_type& native_socket,
                                   boost::system::error_code& ec) {
    impl = native_socket;
    return ec;
  }

  bool is_open(const implementation_type& impl) const {
    return impl.p_next_layer_socket->is_open();
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
    return impl.p_next_layer_socket->close(ec);
  }

  native_type native(implementation_type& impl) { return impl; }

  native_handle_type native_handle(implementation_type& impl) { return impl; }

  boost::system::error_code cancel(implementation_type& impl,
                                   boost::system::error_code& ec) {
    return impl.p_next_layer_socket->cancel(ec);
  }

  bool at_mark(const implementation_type& impl,
               boost::system::error_code& ec) const {
    return impl.p_next_layer_socket->at_mark(ec);
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
                                 const endpoint_type& endpoint,
                                 boost::system::error_code& ec) {
    impl.p_local_endpoint = std::make_shared<endpoint_type>(endpoint);
    return impl.p_next_layer_socket->bind(endpoint.next_layer_endpoint(), ec);
  }

  boost::system::error_code connect(implementation_type& impl,
                                    const endpoint_type& peer_endpoint,
                                    boost::system::error_code& ec) {
    impl.p_remote_endpoint = std::make_shared<endpoint_type>(peer_endpoint);

    impl.p_next_layer_socket->connect(peer_endpoint.next_layer_endpoint(), ec);

    if (!ec) {
      impl.p_local_endpoint = std::make_shared<endpoint_type>(
          impl.p_next_layer_socket->local_endpoint(ec));
    }

    return ec;
  }

  template <typename ConnectHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(ConnectHandler, void(boost::system::error_code))
      async_connect(implementation_type& impl,
                    const endpoint_type& peer_endpoint,
                    ConnectHandler&& handler) {
    boost::asio::detail::async_result_init<ConnectHandler,
                                           void(boost::system::error_code)>
        init(std::forward<ConnectHandler>(handler));

    impl.p_remote_endpoint = std::make_shared<endpoint_type>(peer_endpoint);

    auto fill_local_endpoint_lambda =
        [&impl, init](const boost::system::error_code& ec) mutable {
      if (!ec) {
        boost::system::error_code local_ec;
        impl.p_local_endpoint = std::make_shared<endpoint_type>(
            impl.p_next_layer_socket->local_endpoint(local_ec));
      }

      init.handler(ec);
    };

    impl.p_next_layer_socket->async_connect(
        peer_endpoint.next_layer_endpoint(),
        io::ComposedOp<decltype(fill_local_endpoint_lambda),
                       typename boost::asio::handler_type<
                           ConnectHandler, void(boost::system::error_code)>::
                           type>(std::move(fill_local_endpoint_lambda),
                                 init.handler));

    return init.result.get();
  }

  template <typename ConstBufferSequence, typename WriteHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler,
                                void(boost::system::error_code, std::size_t))
      async_send(implementation_type& impl, const ConstBufferSequence& buffers,
                 boost::asio::socket_base::message_flags flags,
                 WriteHandler&& handler) {
    return impl.p_next_layer_socket->async_send(
        buffers, std::forward<WriteHandler>(handler));
  }

  template <typename MutableBufferSequence, typename ReadHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler,
                                void(boost::system::error_code, std::size_t))
      async_receive(implementation_type& impl,
                    const MutableBufferSequence& buffers,
                    boost::asio::socket_base::message_flags flags,
                    ReadHandler&& handler) {
    return impl.p_next_layer_socket->async_receive(
        buffers, std::forward<ReadHandler>(handler));
  }

  boost::system::error_code shutdown(
      implementation_type& impl, boost::asio::socket_base::shutdown_type what,
      boost::system::error_code& ec) {
    impl.p_next_layer_socket->shutdown(what, ec);

    return ec;
  }

 private:
  void shutdown_service() {}
};

template <class Protocol>
class VirtualEmptyStreamAcceptor_service
    : public boost::asio::detail::service_base<
          VirtualEmptyStreamAcceptor_service<Protocol>> {
 public:
  /// The protocol type.
  typedef Protocol protocol_type;
  /// The endpoint type.
  typedef typename protocol_type::endpoint endpoint_type;

  typedef basic_acceptor_impl<protocol_type> implementation_type;
  typedef implementation_type& native_handle_type;
  typedef native_handle_type native_type;

 private:
  typedef typename protocol_type::next_layer_protocol::acceptor
      next_acceptor_type;

 public:
  explicit VirtualEmptyStreamAcceptor_service(
      boost::asio::io_service& io_service)
      : boost::asio::detail::service_base<VirtualEmptyStreamAcceptor_service>(
            io_service) {}

  virtual ~VirtualEmptyStreamAcceptor_service() {}

  void construct(implementation_type& impl) {
    impl.p_next_layer_acceptor =
        std::make_shared<next_acceptor_type>(this->get_io_service());
  }

  void destroy(implementation_type& impl) {
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
                                   const native_handle_type& native_socket,
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
    impl.p_next_layer_acceptor->accept(
        *peer.native_handle().p_next_layer_socket,
        p_peer_endpoint->next_layer_endpoint(), ec);

    if (!ec) {
      auto& peer_impl = peer.native_handle();
      peer_impl.p_local_endpoint = impl.p_local_endpoint;
      // Add current layer endpoint context here (if necessary)
      p_peer_endpoint->set();
      peer_impl.p_remote_endpoint =
          std::make_shared<Protocol1::endpoint>(*p_peer_endpoint);
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
    peer_impl.p_local_endpoint = impl.p_local_endpoint;
    peer_impl.p_remote_endpoint = std::make_shared<typename Protocol1::endpoint>();

    detail::AcceptOp<
        protocol_type, next_acceptor_type,
        typename boost::asio::basic_socket<Protocol1, SocketService>::native_handle_type,
        endpoint_type,
        typename boost::asio::
            handler_type<AcceptHandler, void(boost::system::error_code)>::type>(
        *impl.p_next_layer_acceptor, peer_impl, p_peer_endpoint,
        init.handler)();

    return init.result.get();
  }

 private:
  void shutdown_service() {}
};

#include <boost/asio/detail/pop_options.hpp>

template <class NextLayer>
using VirtualEmptyStream_endpoint =
    typename VirtualEmptyStreamProtocol<NextLayer>::endpoint;
template <class NextLayer>
using VirtualEmptyStream_socket =
    typename VirtualEmptyStreamProtocol<NextLayer>::socket;
template <class NextLayer>
using VirtualEmptyStream_acceptor =
    typename VirtualEmptyStreamProtocol<NextLayer>::acceptor;
template <class NextLayer>
using VirtualEmptyStream_resolver =
    typename VirtualEmptyStreamProtocol<NextLayer>::resolver;

}  // virtual_network

#endif  // SSF_CORE_VIRTUAL_NETWORK_BASIC_EMPTY_STREAM_H_

#ifndef SSF_LAYER_PROXY_BASIC_PROXY_ACCEPTOR_SERVICE_H_
#define SSF_LAYER_PROXY_BASIC_PROXY_ACCEPTOR_SERVICE_H_

#include <map>
#include <memory>
#include <queue>
#include <set>
#include <utility>

#include <boost/asio/async_result.hpp>
#include <boost/asio/basic_stream_socket.hpp>
#include <boost/asio/basic_socket_acceptor.hpp>
#include <boost/asio/detail/config.hpp>
#include <boost/asio/io_service.hpp>

#include <boost/system/error_code.hpp>
#include <boost/property_tree/ptree.hpp>

#include <boost/bind.hpp>
#include <boost/system/error_code.hpp>
#include <boost/thread.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include "ssf/error/error.h"

#include "ssf/layer/accept_op.h"
#include "ssf/layer/basic_impl.h"
#include "ssf/layer/basic_resolver.h"
#include "ssf/layer/parameters.h"

#include "ssf/network/base_session.h"
#include "ssf/network/manager.h"
#include "ssf/network/session_forwarder.h"

namespace ssf {
namespace layer {
namespace proxy {

#include <boost/asio/detail/push_options.hpp>

template <class Protocol>
class basic_ProxyAcceptor_service : public boost::asio::detail::service_base<
                                        basic_ProxyAcceptor_service<Protocol>> {
 public:
  /// The protocol type.
  typedef Protocol protocol_type;
  /// The endpoint type.
  typedef typename protocol_type::endpoint endpoint_type;

  typedef basic_acceptor_impl<protocol_type> implementation_type;
  typedef implementation_type& native_handle_type;
  typedef native_handle_type native_type;

 private:
  typedef
      typename protocol_type::next_layer_protocol::acceptor next_acceptor_type;

 public:
  explicit basic_ProxyAcceptor_service(boost::asio::io_service& io_service)
      : boost::asio::detail::service_base<basic_ProxyAcceptor_service>(
            io_service) {}

  virtual ~basic_ProxyAcceptor_service() {}

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
                                       const SettableSocketOption& option,
                                       boost::system::error_code& ec) {
    if (impl.p_next_layer_acceptor) {
      return impl.p_next_layer_acceptor->set_option(option, ec);
    }
    return ec;
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
    peer_impl.p_remote_endpoint =
        std::make_shared<typename Protocol1::endpoint>();

    impl.p_next_layer_acceptor->accept(
        *peer.native_handle().p_next_layer_socket,
        peer_impl.p_remote_endpoint->next_layer_endpoint(), ec);

    if (!ec) {
      peer_impl.p_local_endpoint = impl.p_local_endpoint;

      // Add current layer endpoint context here (if necessary)
      peer_impl.p_remote_endpoint->set();

      if (p_peer_endpoint) {
        *p_peer_endpoint = *(peer_impl.p_remote_endpoint);
      }
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
    peer_impl.p_local_endpoint =
        std::make_shared<typename Protocol1::endpoint>();
    peer_impl.p_remote_endpoint =
        std::make_shared<typename Protocol1::endpoint>();

    ssf::layer::detail::AcceptOp<
        protocol_type, next_acceptor_type,
        typename std::remove_reference<typename boost::asio::basic_socket<
            Protocol1, SocketService>::native_handle_type>::type,
        endpoint_type,
        typename boost::asio::handler_type<
            AcceptHandler, void(boost::system::error_code)>::type> (
        *impl.p_next_layer_acceptor, &peer_impl, p_peer_endpoint,
        init.handler)();

    return init.result.get();
  }

 private:
  void shutdown_service() {}
};

#include <boost/asio/detail/pop_options.hpp>

}  // proxy
}  // layer
}  // ssf

#endif  // SSF_LAYER_PROXY_BASIC_PROXY_ACCEPTOR_SERVICE_H_

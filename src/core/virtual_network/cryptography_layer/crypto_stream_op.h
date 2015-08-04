#ifndef SSF_CORE_VIRTUAL_NETWORK_CRYPTOGRAPHY_LAYER_CRYPTO_STREAM_OP_H_
#define SSF_CORE_VIRTUAL_NETWORK_CRYPTOGRAPHY_LAYER_CRYPTO_STREAM_OP_H_

#include <type_traits>

#include <boost/asio/coroutine.hpp>
#include <boost/asio/detail/handler_alloc_helpers.hpp>
#include <boost/asio/detail/handler_cont_helpers.hpp>
#include <boost/asio/detail/handler_invoke_helpers.hpp>

#include <boost/system/error_code.hpp>

namespace virtual_network {
namespace cryptography_layer {
namespace detail {

template <class CryptoProtocol, class Stream, class Endpoint,
          class ConnectHandler>
class CryptoStreamConnectOp {
 public:
  CryptoStreamConnectOp(Stream& stream, Endpoint* p_local_endpoint,
                        const Endpoint& peer_endpoint,
                        const ConnectHandler& handler)
      : coro_(),
        stream_(stream),
        p_local_endpoint_(p_local_endpoint),
        peer_endpoint_(peer_endpoint),
        handler_(handler) {}

  CryptoStreamConnectOp(const CryptoStreamConnectOp& other)
      : coro_(other.coro_),
        stream_(other.stream_),
        p_local_endpoint_(other.p_local_endpoint_),
        peer_endpoint_(other.peer_endpoint_),
        handler_(other.handler_) {}

  CryptoStreamConnectOp(CryptoStreamConnectOp&& other)
      : coro_(std::move(other.coro_)),
        stream_(other.stream_),
        p_local_endpoint_(std::move(other.p_local_endpoint_)),
        peer_endpoint_(std::move(other.peer_endpoint_)),
        handler_(std::move(other.handler_)) {}

#include <boost/asio/yield.hpp>
  void operator()(
      const boost::system::error_code& ec = boost::system::error_code()) {
    if (!ec) {
      reenter(coro_) {
        yield stream_.next_layer().async_connect(
            peer_endpoint_.next_layer_endpoint(), std::move(*this));

        boost::system::error_code endpoint_ec;
        p_local_endpoint_->next_layer_endpoint() =
            stream_.next_layer().local_endpoint(endpoint_ec);
        p_local_endpoint_->set();

        stream_.async_handshake(CryptoProtocol::handshake_type::client,
                                std::move(handler_));
      }
    } else {
      // error
      handler_(ec);
    }
  }
#include <boost/asio/unyield.hpp>

  inline ConnectHandler& handler() { return handler_; }

 private:
  boost::asio::coroutine coro_;
  Stream& stream_;
  Endpoint* p_local_endpoint_;
  Endpoint peer_endpoint_;
  ConnectHandler handler_;
};

template <class CryptoProtocol, class Stream, class Endpoint,
          class ConnectHandler>
inline void* asio_handler_allocate(
    std::size_t size, CryptoStreamConnectOp<CryptoProtocol, Stream, Endpoint,
                                            ConnectHandler>* this_handler) {
  return boost_asio_handler_alloc_helpers::allocate(size,
                                                    this_handler->handler());
}

template <class CryptoProtocol, class Stream, class Endpoint,
          class ConnectHandler>
inline void asio_handler_deallocate(
    void* pointer, std::size_t size,
    CryptoStreamConnectOp<CryptoProtocol, Stream, Endpoint, ConnectHandler>*
        this_handler) {
  boost_asio_handler_alloc_helpers::deallocate(pointer, size,
                                               this_handler->handler());
}

template <class CryptoProtocol, class Stream, class Endpoint,
          class ConnectHandler>
inline bool asio_handler_is_continuation(CryptoStreamConnectOp<
    CryptoProtocol, Stream, Endpoint, ConnectHandler>* this_handler) {
  return boost_asio_handler_cont_helpers::is_continuation(
      this_handler->handler());
}

template <class Function, class CryptoProtocol, class Stream, class Endpoint,
          class ConnectHandler>
inline void asio_handler_invoke(
    Function& function, CryptoStreamConnectOp<CryptoProtocol, Stream, Endpoint,
                                              ConnectHandler>* this_handler) {
  boost_asio_handler_invoke_helpers::invoke(function, this_handler->handler());
}

template <class Function, class CryptoProtocol, class Stream, class Endpoint,
          class ConnectHandler>
inline void asio_handler_invoke(
    const Function& function,
    CryptoStreamConnectOp<CryptoProtocol, Stream, Endpoint, ConnectHandler>*
        this_handler) {
  boost_asio_handler_invoke_helpers::invoke(function, this_handler->handler());
}

template <class CryptoProtocol, class Acceptor, class PeerImpl, class Endpoint,
          class AcceptHandler>
class CryptoStreamAcceptOp {
 public:
  CryptoStreamAcceptOp(Acceptor& acceptor, PeerImpl* p_peer_impl,
                       Endpoint* p_peer_endpoint, const AcceptHandler& handler)
      : coro_(),
        acceptor_(acceptor),
        p_peer_impl_(p_peer_impl),
        p_peer_endpoint_(p_peer_endpoint),
        handler_(handler) {}

  CryptoStreamAcceptOp(const CryptoStreamAcceptOp& other)
      : coro_(other.coro_),
        acceptor_(other.acceptor_),
        p_peer_impl_(other.p_peer_impl_),
        p_peer_endpoint_(other.p_peer_endpoint_),
        handler_(other.handler_) {}

  CryptoStreamAcceptOp(CryptoStreamAcceptOp&& other)
      : coro_(std::move(other.coro_)),
        acceptor_(other.acceptor_),
        p_peer_impl_(std::move(other.p_peer_impl_)),
        p_peer_endpoint_(std::move(other.p_peer_endpoint_)),
        handler_(std::move(other.handler_)) {}

#include <boost/asio/yield.hpp>
  void operator()(
      const boost::system::error_code& ec = boost::system::error_code()) {
    if (!ec) {
      reenter(coro_) {
        yield acceptor_.async_accept(
            p_peer_impl_->p_next_layer_socket->next_layer(),
            p_peer_impl_->p_remote_endpoint->next_layer_endpoint(),
            std::move(*this));

        p_peer_impl_->p_remote_endpoint->set();

        p_peer_impl_->p_local_endpoint->next_layer_endpoint() =
            p_peer_impl_->p_next_layer_socket->next_layer().local_endpoint();
        p_peer_impl_->p_local_endpoint->set();

        if (p_peer_endpoint_) {
          *p_peer_endpoint_ = *p_peer_impl_->p_remote_endpoint;
        }

        p_peer_impl_->p_next_layer_socket->async_handshake(
            CryptoProtocol::handshake_type::server, std::move(handler_));
      }
    } else {
      // error
      handler_(ec);
    }
  }
#include <boost/asio/unyield.hpp>

  inline AcceptHandler& handler() { return handler_; }

 private:
  boost::asio::coroutine coro_;
  Acceptor& acceptor_;
  PeerImpl* p_peer_impl_;
  Endpoint* p_peer_endpoint_;
  AcceptHandler handler_;
};

template <class CryptoProtocol, class Acceptor, class PeerImpl, class Endpoint,
          class AcceptHandler>
inline void* asio_handler_allocate(
    std::size_t size,
    CryptoStreamAcceptOp<CryptoProtocol, Acceptor, PeerImpl, Endpoint,
                         AcceptHandler>* this_handler) {
  return boost_asio_handler_alloc_helpers::allocate(size,
                                                    this_handler->handler());
}

template <class CryptoProtocol, class Acceptor, class PeerImpl, class Endpoint,
          class AcceptHandler>
inline void asio_handler_deallocate(
    void* pointer, std::size_t size,
    CryptoStreamAcceptOp<CryptoProtocol, Acceptor, PeerImpl, Endpoint,
                         AcceptHandler>* this_handler) {
  boost_asio_handler_alloc_helpers::deallocate(pointer, size,
                                               this_handler->handler());
}

template <class CryptoProtocol, class Acceptor, class PeerImpl, class Endpoint,
          class AcceptHandler>
inline bool asio_handler_is_continuation(
    CryptoStreamAcceptOp<CryptoProtocol, Acceptor, PeerImpl, Endpoint,
                         AcceptHandler>* this_handler) {
  return boost_asio_handler_cont_helpers::is_continuation(
      this_handler->handler());
}

template <class Function, class CryptoProtocol, class Acceptor, class PeerImpl,
          class Endpoint, class AcceptHandler>
inline void asio_handler_invoke(
    Function& function,
    CryptoStreamAcceptOp<CryptoProtocol, Acceptor, PeerImpl, Endpoint,
                         AcceptHandler>* this_handler) {
  boost_asio_handler_invoke_helpers::invoke(function, this_handler->handler());
}

template <class Function, class CryptoProtocol, class Acceptor, class PeerImpl,
          class Endpoint, class AcceptHandler>
inline void asio_handler_invoke(
    const Function& function,
    CryptoStreamAcceptOp<CryptoProtocol, Acceptor, PeerImpl, Endpoint,
                         AcceptHandler>* this_handler) {
  boost_asio_handler_invoke_helpers::invoke(function, this_handler->handler());
}

}  // detail
}  // cryptography_layer
}  // virtual_network

#endif  // SSF_CORE_VIRTUAL_NETWORK_CRYPTOGRAPHY_LAYER_CRYPTO_STREAM_OP_H_

#ifndef SSF_CORE_VIRTUAL_NETWORK_DATA_LINK_LAYER_CIRCUIT_OP_H_
#define SSF_CORE_VIRTUAL_NETWORK_DATA_LINK_LAYER_CIRCUIT_OP_H_

#include <type_traits>

#include <boost/system/error_code.hpp>

#include <boost/asio/coroutine.hpp>

#include <boost/asio/detail/handler_alloc_helpers.hpp>
#include <boost/asio/detail/handler_cont_helpers.hpp>
#include <boost/asio/detail/handler_invoke_helpers.hpp>

namespace virtual_network {
namespace data_link_layer {
namespace detail {

template <class Protocol, class Stream, class Endpoint,
          class ConnectHandler>
class CircuitConnectOp {
 public:
  CircuitConnectOp(Stream& stream, Endpoint* p_local_endpoint,
                   Endpoint* p_remote_endpoint, ConnectHandler handler)
      : coro_(),
        stream_(stream),
        p_local_endpoint_(p_local_endpoint),
        p_remote_endpoint_(p_remote_endpoint),
        handler_(std::move(handler)) {}

  CircuitConnectOp(const CircuitConnectOp& other)
      : coro_(other.coro_),
        stream_(other.stream_),
        p_local_endpoint_(other.p_local_endpoint_),
        p_remote_endpoint_(other.p_remote_endpoint_),
        handler_(other.handler_) {}

  CircuitConnectOp(CircuitConnectOp&& other)
      : coro_(std::move(other.coro_)),
        stream_(other.stream_),
        p_local_endpoint_(other.p_local_endpoint_),
        p_remote_endpoint_(other.p_remote_endpoint_),
        handler_(std::move(other.handler_)) {}

#include <boost/asio/yield.hpp>
  void operator()(
      const boost::system::error_code& ec = boost::system::error_code()) {
    if (!ec) {
      reenter(coro_) {
        yield stream_.async_connect(
            p_remote_endpoint_->next_layer_endpoint(), std::move(*this));

        boost::system::error_code endpoint_ec;
        auto& next_layer_endpoint = p_local_endpoint_->next_layer_endpoint();
        next_layer_endpoint = stream_.local_endpoint(endpoint_ec);
        p_local_endpoint_->set();

        Protocol::circuit_policy::AsyncInitConnection(
            stream_, p_remote_endpoint_, Protocol::circuit_policy::client,
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
  Endpoint* p_remote_endpoint_;
  ConnectHandler handler_;
};

template <class Protocol, class StreamPtr, class EndpointPtr,
          class ConnectHandler>
inline void* asio_handler_allocate(
    std::size_t size, CircuitConnectOp<Protocol, StreamPtr, EndpointPtr,
                                       ConnectHandler>* this_handler) {
  return boost_asio_handler_alloc_helpers::allocate(size,
                                                    this_handler->handler());
}

template <class Protocol, class Stream, class Endpoint,
          class ConnectHandler>
inline void asio_handler_deallocate(
    void* pointer, std::size_t size,
    CircuitConnectOp<Protocol, Stream, Endpoint, ConnectHandler>*
        this_handler) {
  boost_asio_handler_alloc_helpers::deallocate(pointer, size,
                                               this_handler->handler());
}

template <class Protocol, class Stream, class Endpoint,
          class ConnectHandler>
inline bool asio_handler_is_continuation(CircuitConnectOp<
    Protocol, Stream, Endpoint, ConnectHandler>* this_handler) {
  return boost_asio_handler_cont_helpers::is_continuation(
      this_handler->handler());
}

template <class Function, class Protocol, class Stream, class Endpoint,
          class ConnectHandler>
inline void asio_handler_invoke(
    Function& function, CircuitConnectOp<Protocol, Stream, Endpoint,
                                         ConnectHandler>* this_handler) {
  boost_asio_handler_invoke_helpers::invoke(function, this_handler->handler());
}

template <class Function, class Protocol, class Stream, class Endpoint,
          class ConnectHandler>
inline void asio_handler_invoke(
    const Function& function, CircuitConnectOp<Protocol, Stream, Endpoint,
                                               ConnectHandler>* this_handler) {
  boost_asio_handler_invoke_helpers::invoke(function, this_handler->handler());
}

template <class PeerImpl, class Endpoint, class AcceptHandler>
class CircuitAcceptOp {
 public:
  CircuitAcceptOp(typename std::remove_reference<PeerImpl>::type& peer_impl,
                  Endpoint* p_peer_endpoint, AcceptHandler handler)
      : peer_impl_(peer_impl),
        p_peer_endpoint_(std::move(p_peer_endpoint)),
        handler_(std::move(handler)) {}

  CircuitAcceptOp(const CircuitAcceptOp& other)
      : peer_impl_(other.peer_impl_),
        p_peer_endpoint_(other.p_peer_endpoint_),
        handler_(other.handler_) {}

  CircuitAcceptOp(CircuitAcceptOp&& other)
      : peer_impl_(other.peer_impl_),
        p_peer_endpoint_(std::move(other.p_peer_endpoint_)),
        handler_(std::move(other.handler_)) {}

#include <boost/asio/yield.hpp>
  void operator()(
      const boost::system::error_code& ec = boost::system::error_code()) {
    if (p_peer_endpoint_) {
      *p_peer_endpoint_ = *peer_impl_.p_remote_endpoint;
    }

    handler_(ec);
  }
#include <boost/asio/unyield.hpp>

  inline AcceptHandler& handler() { return handler_; }

 private:
  PeerImpl& peer_impl_;
  Endpoint* p_peer_endpoint_;
  AcceptHandler handler_;
};

template <class PeerImpl, class Endpoint, class AcceptHandler>
inline void* asio_handler_allocate(
    std::size_t size,
    CircuitAcceptOp<PeerImpl, Endpoint, AcceptHandler>* this_handler) {
  return boost_asio_handler_alloc_helpers::allocate(size,
                                                    this_handler->handler());
}

template <class PeerImpl, class Endpoint, class AcceptHandler>
inline void asio_handler_deallocate(
    void* pointer, std::size_t size,
    CircuitAcceptOp<PeerImpl, Endpoint, AcceptHandler>* this_handler) {
  boost_asio_handler_alloc_helpers::deallocate(pointer, size,
                                               this_handler->handler());
}

template <class PeerImpl, class Endpoint, class AcceptHandler>
inline bool asio_handler_is_continuation(
    CircuitAcceptOp<PeerImpl, Endpoint, AcceptHandler>* this_handler) {
  return boost_asio_handler_cont_helpers::is_continuation(
      this_handler->handler());
}

template <class Function, class PeerImpl, class Endpoint, class AcceptHandler>
inline void asio_handler_invoke(
    Function& function,
    CircuitAcceptOp<PeerImpl, Endpoint, AcceptHandler>* this_handler) {
  boost_asio_handler_invoke_helpers::invoke(function, this_handler->handler());
}

template <class Function, class PeerImpl, class Endpoint, class AcceptHandler>
inline void asio_handler_invoke(
    const Function& function,
    CircuitAcceptOp<PeerImpl, Endpoint, AcceptHandler>* this_handler) {
  boost_asio_handler_invoke_helpers::invoke(function, this_handler->handler());
}

}  // detail
}  // data_link_layer
}  // virtual_network

#endif  // SSF_CORE_VIRTUAL_NETWORK_DATA_LINK_LAYER_CIRCUIT_OP_H_

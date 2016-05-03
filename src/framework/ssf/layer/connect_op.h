#ifndef SSF_LAYER_CONNECT_OP_H_
#define SSF_LAYER_CONNECT_OP_H_

#include <type_traits>

#include <boost/system/error_code.hpp>

#include <boost/asio/coroutine.hpp>

#include <boost/asio/detail/handler_alloc_helpers.hpp>
#include <boost/asio/detail/handler_cont_helpers.hpp>
#include <boost/asio/detail/handler_invoke_helpers.hpp>

namespace ssf {
namespace layer {
namespace detail {

template <class Stream, class Endpoint>
class ConnectOp {
 public:
  ConnectOp(Stream& stream, Endpoint* p_local_endpoint, Endpoint peer_endpoint)
      : stream_(stream),
        p_local_endpoint_(p_local_endpoint),
        peer_endpoint_(std::move(peer_endpoint)) {}

  void operator()(boost::system::error_code& ec) {
    stream_.connect(peer_endpoint_.next_layer_endpoint(), ec);
    if (!ec) {
      auto& next_layer_endpoint = p_local_endpoint_->next_layer_endpoint();
      next_layer_endpoint = stream_.local_endpoint(ec);
      p_local_endpoint_->set();
    }
  }

 private:
  Stream& stream_;
  Endpoint* p_local_endpoint_;
  Endpoint peer_endpoint_;
};

template <class Protocol, class Stream, class Endpoint, class ConnectHandler>
class AsyncConnectOp {
 public:
  AsyncConnectOp(Stream& stream, Endpoint* p_local_endpoint,
                 Endpoint peer_endpoint, ConnectHandler handler)
      : coro_(),
        stream_(stream),
        p_local_endpoint_(p_local_endpoint),
        peer_endpoint_(std::move(peer_endpoint)),
        handler_(std::move(handler)) {}

  AsyncConnectOp(const AsyncConnectOp& other)
      : coro_(other.coro_),
        stream_(other.stream_),
        p_local_endpoint_(other.p_local_endpoint_),
        peer_endpoint_(other.peer_endpoint_),
        handler_(other.handler_) {}

  AsyncConnectOp(AsyncConnectOp&& other)
      : coro_(std::move(other.coro_)),
        stream_(other.stream_),
        p_local_endpoint_(other.p_local_endpoint_),
        peer_endpoint_(std::move(other.peer_endpoint_)),
        handler_(std::move(other.handler_)) {}

#include <boost/asio/yield.hpp>
  void operator()(
      const boost::system::error_code& ec = boost::system::error_code()) {
    if (!ec) {
      reenter(coro_) {
        yield stream_.async_connect(peer_endpoint_.next_layer_endpoint(),
                                    std::move(*this));

        boost::system::error_code endpoint_ec;
        auto& next_layer_endpoint = p_local_endpoint_->next_layer_endpoint();
        next_layer_endpoint = stream_.local_endpoint(endpoint_ec);
        p_local_endpoint_->set();

        handler_(ec);
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

template <class Protocol, class Stream, class Endpoint, class ConnectHandler>
inline void* asio_handler_allocate(
    std::size_t size,
    AsyncConnectOp<Protocol, Stream, Endpoint, ConnectHandler>* this_handler) {
  return boost_asio_handler_alloc_helpers::allocate(size,
                                                    this_handler->handler());
}

template <class Protocol, class Stream, class Endpoint, class ConnectHandler>
inline void asio_handler_deallocate(
    void* pointer, std::size_t size,
    AsyncConnectOp<Protocol, Stream, Endpoint, ConnectHandler>* this_handler) {
  boost_asio_handler_alloc_helpers::deallocate(pointer, size,
                                               this_handler->handler());
}

template <class Protocol, class Stream, class Endpoint, class ConnectHandler>
inline bool asio_handler_is_continuation(
    AsyncConnectOp<Protocol, Stream, Endpoint, ConnectHandler>* this_handler) {
  return boost_asio_handler_cont_helpers::is_continuation(
      this_handler->handler());
}

template <class Function, class Protocol, class Stream, class Endpoint,
          class ConnectHandler>
inline void asio_handler_invoke(
    Function& function,
    AsyncConnectOp<Protocol, Stream, Endpoint, ConnectHandler>* this_handler) {
  boost_asio_handler_invoke_helpers::invoke(function, this_handler->handler());
}

template <class Function, class Protocol, class Stream, class Endpoint,
          class ConnectHandler>
inline void asio_handler_invoke(
    const Function& function,
    AsyncConnectOp<Protocol, Stream, Endpoint, ConnectHandler>* this_handler) {
  boost_asio_handler_invoke_helpers::invoke(function, this_handler->handler());
}

}  // detail
}  // layer
}  // ssf

#endif  // SSF_LAYER_CONNECT_OP_H_

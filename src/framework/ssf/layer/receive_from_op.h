#ifndef SSF_LAYER_RECEIVE_FROM_OP_H_
#define SSF_LAYER_RECEIVE_FROM_OP_H_

#include <type_traits>

#include <boost/system/error_code.hpp>

#include <boost/asio/coroutine.hpp>

#include <boost/asio/detail/handler_alloc_helpers.hpp>
#include <boost/asio/detail/handler_cont_helpers.hpp>
#include <boost/asio/detail/handler_invoke_helpers.hpp>

#include <boost/asio/socket_base.hpp>

namespace ssf {
namespace layer {
namespace detail {

template <class Protocol, class Socket, class Endpoint,
          class MutableBufferSequence, class ReadHandler>
class ReceiveFromOp {
 public:
  ReceiveFromOp(Socket& socket, Endpoint* p_remote_endpoint,
                MutableBufferSequence buffers,
                boost::asio::socket_base::message_flags flags,
                ReadHandler handler)
      : coro_(),
        socket_(socket),
        p_remote_endpoint_(p_remote_endpoint),
        buffers_(std::move(buffers)),
        flags_(std::move(flags)),
        handler_(std::move(handler)) {}

  ReceiveFromOp(const ReceiveFromOp& other)
      : coro_(other.coro_),
        socket_(other.socket_),
        p_remote_endpoint_(other.p_remote_endpoint_),
        buffers_(other.buffers_),
        flags_(other.flags_),
        handler_(other.handler_) {}

  ReceiveFromOp(ReceiveFromOp&& other)
      : coro_(std::move(other.coro_)),
        socket_(other.socket_),
        p_remote_endpoint_(other.p_remote_endpoint_),
        buffers_(std::move(other.buffers_)),
        flags_(std::move(other.flags_)),
        handler_(std::move(other.handler_)) {}

#include <boost/asio/yield.hpp>
  void operator()(
      const boost::system::error_code& ec = boost::system::error_code(),
      std::size_t length = 0) {
    if (!ec) {
      reenter(coro_) {
        yield socket_.async_receive_from(
            buffers_, p_remote_endpoint_->next_layer_endpoint(), flags_,
            std::move(*this));

        p_remote_endpoint_->set();

        handler_(ec, length);
      }
    } else {
      // error
      handler_(ec, length);
    }
  }
#include <boost/asio/unyield.hpp>

  inline ReadHandler& handler() { return handler_; }

 private:
  boost::asio::coroutine coro_;
  Socket& socket_;
  Endpoint* p_remote_endpoint_;
  MutableBufferSequence buffers_;
  boost::asio::socket_base::message_flags flags_;
  ReadHandler handler_;
};

template <class Protocol, class Socket, class Endpoint,
          class MutableBufferSequence, class ReadHandler>
inline void* asio_handler_allocate(
    std::size_t size,
    ReceiveFromOp<Protocol, Socket, Endpoint, MutableBufferSequence,
                  ReadHandler>* this_handler) {
  return boost_asio_handler_alloc_helpers::allocate(size,
                                                    this_handler->handler());
}

template <class Protocol, class Socket, class Endpoint,
          class MutableBufferSequence, class ReadHandler>
inline void asio_handler_deallocate(
    void* pointer, std::size_t size,
    ReceiveFromOp<Protocol, Socket, Endpoint, MutableBufferSequence,
                  ReadHandler>* this_handler) {
  boost_asio_handler_alloc_helpers::deallocate(pointer, size,
                                               this_handler->handler());
}

template <class Protocol, class Socket, class Endpoint,
          class MutableBufferSequence, class ReadHandler>
inline bool asio_handler_is_continuation(
    ReceiveFromOp<Protocol, Socket, Endpoint, MutableBufferSequence,
                  ReadHandler>* this_handler) {
  return boost_asio_handler_cont_helpers::is_continuation(
      this_handler->handler());
}

template <class Function, class Protocol, class Socket, class Endpoint,
          class MutableBufferSequence, class ReadHandler>
inline void asio_handler_invoke(
    Function& function,
    ReceiveFromOp<Protocol, Socket, Endpoint, MutableBufferSequence,
                  ReadHandler>* this_handler) {
  boost_asio_handler_invoke_helpers::invoke(function, this_handler->handler());
}

template <class Function, class Protocol, class Socket, class Endpoint,
          class MutableBufferSequence, class ReadHandler>
inline void asio_handler_invoke(
    const Function& function,
    ReceiveFromOp<Protocol, Socket, Endpoint, MutableBufferSequence,
                  ReadHandler>* this_handler) {
  boost_asio_handler_invoke_helpers::invoke(function, this_handler->handler());
}

}  // detail
}  // layer
}  // ssf

#endif  // SSF_LAYER_RECEIVE_FROM_OP_H_

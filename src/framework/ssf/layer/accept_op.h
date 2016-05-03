#ifndef SSF_LAYER_ACCEPT_OP_H_
#define SSF_LAYER_ACCEPT_OP_H_

#include <type_traits>

#include <boost/system/error_code.hpp>

#include <boost/asio/coroutine.hpp>

#include <boost/asio/detail/handler_alloc_helpers.hpp>
#include <boost/asio/detail/handler_cont_helpers.hpp>
#include <boost/asio/detail/handler_invoke_helpers.hpp>

namespace ssf {
namespace layer {
namespace detail {

template <class Protocol, class Acceptor, class PeerImpl, class Endpoint,
          class AcceptHandler>
class AcceptOp {
 public:
  AcceptOp(Acceptor& acceptor, PeerImpl* p_peer_impl, Endpoint* p_peer_endpoint,
           AcceptHandler handler)
      : coro_(),
        acceptor_(acceptor),
        p_peer_impl_(p_peer_impl),
        p_peer_endpoint_(p_peer_endpoint),
        handler_(std::move(handler)) {}

  AcceptOp(const AcceptOp& other)
      : coro_(other.coro_),
        acceptor_(other.acceptor_),
        p_peer_impl_(other.p_peer_impl_),
        p_peer_endpoint_(other.p_peer_endpoint_),
        handler_(other.handler_) {}

  AcceptOp(AcceptOp&& other)
      : coro_(std::move(other.coro_)),
        acceptor_(other.acceptor_),
        p_peer_impl_(other.p_peer_impl_),
        p_peer_endpoint_(other.p_peer_endpoint_),
        handler_(std::move(other.handler_)) {}

#include <boost/asio/yield.hpp>
  void operator()(
      const boost::system::error_code& ec = boost::system::error_code()) {
    if (!ec) {
      reenter(coro_) {
        yield acceptor_.async_accept(
            *p_peer_impl_->p_next_layer_socket,
            p_peer_impl_->p_remote_endpoint->next_layer_endpoint(),
            std::move(*this));

        p_peer_impl_->p_remote_endpoint->set();

        auto& local_endpoint = *p_peer_impl_->p_local_endpoint;
        local_endpoint =
            Endpoint(local_endpoint.endpoint_context(),
                     p_peer_impl_->p_next_layer_socket->local_endpoint());

        if (p_peer_endpoint_) {
          *p_peer_endpoint_ = *p_peer_impl_->p_remote_endpoint;
        }

        handler_(ec);
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

template <class Protocol, class Acceptor, class PeerImpl, class Endpoint,
          class AcceptHandler>
inline void* asio_handler_allocate(
    std::size_t size, AcceptOp<Protocol, Acceptor, PeerImpl, Endpoint,
                               AcceptHandler>* this_handler) {
  return boost_asio_handler_alloc_helpers::allocate(size,
                                                    this_handler->handler());
}

template <class Protocol, class Acceptor, class PeerImpl, class Endpoint,
          class AcceptHandler>
inline void asio_handler_deallocate(
    void* pointer, std::size_t size,
    AcceptOp<Protocol, Acceptor, PeerImpl, Endpoint, AcceptHandler>*
        this_handler) {
  boost_asio_handler_alloc_helpers::deallocate(pointer, size,
                                               this_handler->handler());
}

template <class Protocol, class Acceptor, class PeerImpl, class Endpoint,
          class AcceptHandler>
inline bool asio_handler_is_continuation(AcceptOp<
    Protocol, Acceptor, PeerImpl, Endpoint, AcceptHandler>* this_handler) {
  return boost_asio_handler_cont_helpers::is_continuation(
      this_handler->handler());
}

template <class Function, class Protocol, class Acceptor, class PeerImpl,
          class Endpoint, class AcceptHandler>
inline void asio_handler_invoke(Function& function,
                                AcceptOp<Protocol, Acceptor, PeerImpl, Endpoint,
                                         AcceptHandler>* this_handler) {
  boost_asio_handler_invoke_helpers::invoke(function, this_handler->handler());
}

template <class Function, class Protocol, class Acceptor, class PeerImpl,
          class Endpoint, class AcceptHandler>
inline void asio_handler_invoke(const Function& function,
                                AcceptOp<Protocol, Acceptor, PeerImpl, Endpoint,
                                         AcceptHandler>* this_handler) {
  boost_asio_handler_invoke_helpers::invoke(function, this_handler->handler());
}

}  // detail
}  // layer
}  // ssf

#endif  // SSF_LAYER_ACCEPT_OP_H_

#ifndef SSF_LAYER_PROXY_HTTP_CONNECT_OP_H_
#define SSF_LAYER_PROXY_HTTP_CONNECT_OP_H_

#include <exception>
#include <memory>
#include <string>
#include <type_traits>

#include <boost/asio/coroutine.hpp>
#include <boost/asio/detail/handler_alloc_helpers.hpp>
#include <boost/asio/detail/handler_cont_helpers.hpp>
#include <boost/asio/detail/handler_invoke_helpers.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>

#include <boost/system/error_code.hpp>

namespace ssf {
namespace layer {
namespace proxy {
namespace detail {

std::string GenerateHttpProxyRequest(const std::string& target_addr,
                                     const std::string& target_port);

bool CheckHttpProxyResponse(boost::asio::streambuf& streambuf,
                            std::size_t response_size);

template <class Stream, class Endpoint>
class HttpConnectOp {
 public:
  HttpConnectOp(Stream& stream, Endpoint* p_local_endpoint,
                Endpoint peer_endpoint)
      : stream_(stream),
        p_local_endpoint_(p_local_endpoint),
        peer_endpoint_(std::move(peer_endpoint)) {}

  void operator()(boost::system::error_code& ec) {
    stream_.connect(peer_endpoint_.next_layer_endpoint(), ec);

    if (ec) {
      return;
    }

    boost::system::error_code close_ec;
    try {
      auto& next_layer_endpoint = p_local_endpoint_->next_layer_endpoint();
      next_layer_endpoint = stream_.local_endpoint(ec);

      p_local_endpoint_->set();

      // Send connect request to context addr:port
      auto& endpoint_context = peer_endpoint_.endpoint_context();
      auto& target_addr = endpoint_context.http_proxy.addr;
      auto& target_port = endpoint_context.http_proxy.port;

      std::string connect_str =
          GenerateHttpProxyRequest(target_addr, target_port);

      boost::asio::write(
          stream_, boost::asio::buffer(connect_str, connect_str.size()), ec);

      boost::asio::streambuf buffer;
      // Read server response status code 200
      std::size_t read_bytes =
          boost::asio::read_until(stream_, buffer, "\r\n", ec);

      std::string response = std::string(
          boost::asio::buffer_cast<const char*>(buffer.data()), read_bytes);

      // Proxy OK
      if (CheckHttpProxyResponse(buffer, read_bytes)) {
        return;
      } else {
        stream_.close(close_ec);
        ec.assign(ssf::error::broken_pipe, ssf::error::get_ssf_category());
      }
    } catch (const std::exception&) {
      stream_.close(close_ec);
      ec.assign(ssf::error::broken_pipe, ssf::error::get_ssf_category());
      return;
    }
  }

 private:
  Stream& stream_;
  Endpoint* p_local_endpoint_;
  Endpoint peer_endpoint_;
};

template <class Protocol, class Stream, class Endpoint, class ConnectHandler>
class AsyncHttpConnectOp {
 public:
  AsyncHttpConnectOp(Stream& stream, Endpoint* p_local_endpoint,
                     Endpoint peer_endpoint, ConnectHandler handler)
      : coro_(),
        stream_(stream),
        p_local_endpoint_(p_local_endpoint),
        peer_endpoint_(std::move(peer_endpoint)),
        handler_(std::move(handler)),
        p_connect_str_(new std::string("")),
        p_buffer_(new boost::asio::streambuf()) {}

  AsyncHttpConnectOp(const AsyncHttpConnectOp& other)
      : coro_(other.coro_),
        stream_(other.stream_),
        p_local_endpoint_(other.p_local_endpoint_),
        peer_endpoint_(other.peer_endpoint_),
        handler_(other.handler_),
        p_connect_str_(other.p_connect_str_),
        p_buffer_(other.p_buffer_) {}

  AsyncHttpConnectOp(AsyncHttpConnectOp&& other)
      : coro_(std::move(other.coro_)),
        stream_(other.stream_),
        p_local_endpoint_(other.p_local_endpoint_),
        peer_endpoint_(std::move(other.peer_endpoint_)),
        handler_(std::move(other.handler_)),
        p_connect_str_(other.p_connect_str_),
        p_buffer_(other.p_buffer_) {}

#include <boost/asio/yield.hpp>
  void operator()(
      const boost::system::error_code& ec = boost::system::error_code(),
      std::size_t size = 0) {
    if (!ec) {
      boost::system::error_code endpoint_ec;
      auto& next_layer_endpoint = p_local_endpoint_->next_layer_endpoint();
      auto& endpoint_context = peer_endpoint_.endpoint_context();
      auto& target_addr = endpoint_context.http_proxy.addr;
      auto& target_port = endpoint_context.http_proxy.port;

      reenter(coro_) {
        yield stream_.async_connect(peer_endpoint_.next_layer_endpoint(),
                                    std::move(*this));

        next_layer_endpoint = stream_.local_endpoint(endpoint_ec);
        p_local_endpoint_->set();

        // Send connect request to context addr:port
        *p_connect_str_ = GenerateHttpProxyRequest(target_addr, target_port);

        yield boost::asio::async_write(
            stream_,
            boost::asio::buffer(*p_connect_str_, p_connect_str_->size()),
            std::move(*this));

        // Read server response status code 200
        yield boost::asio::async_read_until(stream_, *p_buffer_, "\r\n",
                                            std::move(*this));

        response_ = std::string(
            boost::asio::buffer_cast<const char*>(p_buffer_->data()), size);

        // Proxy OK
        if (CheckHttpProxyResponse(*p_buffer_, size)) {
          handler_(ec);
          return;
        }

        stream_.close(close_ec_);
        close_ec_.assign(ssf::error::broken_pipe,
                         ssf::error::get_ssf_category());
        handler_(close_ec_);
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
  std::shared_ptr<std::string> p_connect_str_;
  std::shared_ptr<boost::asio::streambuf> p_buffer_;
  std::string separator_;
  std::string response_;
  boost::system::error_code close_ec_;
};

template <class Protocol, class Stream, class Endpoint, class ConnectHandler>
inline void* asio_handler_allocate(
    std::size_t size, AsyncHttpConnectOp<Protocol, Stream, Endpoint,
                                         ConnectHandler>* this_handler) {
  return boost_asio_handler_alloc_helpers::allocate(size,
                                                    this_handler->handler());
}

template <class Protocol, class Stream, class Endpoint, class ConnectHandler>
inline void asio_handler_deallocate(
    void* pointer, std::size_t size,
    AsyncHttpConnectOp<Protocol, Stream, Endpoint, ConnectHandler>*
        this_handler) {
  boost_asio_handler_alloc_helpers::deallocate(pointer, size,
                                               this_handler->handler());
}

template <class Protocol, class Stream, class Endpoint, class ConnectHandler>
inline bool asio_handler_is_continuation(AsyncHttpConnectOp<
    Protocol, Stream, Endpoint, ConnectHandler>* this_handler) {
  return boost_asio_handler_cont_helpers::is_continuation(
      this_handler->handler());
}

template <class Function, class Protocol, class Stream, class Endpoint,
          class ConnectHandler>
inline void asio_handler_invoke(
    Function& function, AsyncHttpConnectOp<Protocol, Stream, Endpoint,
                                           ConnectHandler>* this_handler) {
  boost_asio_handler_invoke_helpers::invoke(function, this_handler->handler());
}

template <class Function, class Protocol, class Stream, class Endpoint,
          class ConnectHandler>
inline void asio_handler_invoke(
    const Function& function,
    AsyncHttpConnectOp<Protocol, Stream, Endpoint, ConnectHandler>*
        this_handler) {
  boost_asio_handler_invoke_helpers::invoke(function, this_handler->handler());
}

}  // detail
}  // proxy
}  // layer
}  // ssf

#endif  // SSF_LAYER_PROXY_HTTP_CONNECT_OP_H_

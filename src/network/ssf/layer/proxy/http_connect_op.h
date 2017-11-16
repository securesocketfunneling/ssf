#ifndef SSF_LAYER_PROXY_HTTP_CONNECT_OP_H_
#define SSF_LAYER_PROXY_HTTP_CONNECT_OP_H_

#include <array>
#include <exception>
#include <memory>
#include <string>

#include <boost/asio/coroutine.hpp>
#include <boost/asio/detail/handler_alloc_helpers.hpp>
#include <boost/asio/detail/handler_cont_helpers.hpp>
#include <boost/asio/detail/handler_invoke_helpers.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/asio/write.hpp>

#include <boost/system/error_code.hpp>

#include "ssf/layer/proxy/http_response_builder.h"
#include "ssf/layer/proxy/http_session_initializer.h"

#include "ssf/log/log.h"

namespace ssf {
namespace layer {
namespace proxy {

template <class Stream, class Endpoint>
class HttpConnectOp {
 public:
  using Buffer = std::array<char, 4 * 1024>;

 public:
  HttpConnectOp(Stream& stream, Endpoint* p_local_endpoint,
                Endpoint peer_endpoint)
      : stream_(stream),
        p_local_endpoint_(p_local_endpoint),
        peer_endpoint_(std::move(peer_endpoint)) {}

  void operator()(boost::system::error_code& ec) {
    auto& endpoint_context = peer_endpoint_.endpoint_context();

    p_local_endpoint_->set();

    boost::system::error_code close_ec;
    boost::system::error_code connect_ec;

    try {
      HttpSessionInitializer session_initializer;
      HttpResponseBuilder response_builder;
      Buffer buffer;
      std::size_t bytes_read;

      session_initializer.Reset(endpoint_context.remote_host().addr(),
                                endpoint_context.remote_host().port(),
                                endpoint_context);

      HttpRequest http_request;

      // session initialization (connect request + auth)
      while (session_initializer.status() ==
             HttpSessionInitializer::Status::kContinue) {
        // (re)connect to proxy (new strategy or connection close header)?
        if ((session_initializer.stage() ==
             HttpSessionInitializer::Stage::kConnect) ||
            response_builder.Get()->CloseConnection()) {
          if (stream_.is_open()) {
            stream_.shutdown(boost::asio::socket_base::shutdown_both, ec);
            stream_.close(ec);
          }
          stream_.connect(endpoint_context.http_proxy().ToTcpEndpoint(
              stream_.get_io_service()));
        }

        // send request
        session_initializer.PopulateRequest(&http_request, connect_ec);
        if (connect_ec.value() != 0) {
          SSF_LOG("network_proxy", error,
                  "session initializer could not "
                  "generate connect request");
          break;
        }

        auto request = http_request.Serialize();
        boost::asio::write(stream_, boost::asio::buffer(request), connect_ec);
        if (connect_ec.value() != 0) {
          SSF_LOG("network_proxy", error,
                  "session initializer could not "
                  "process connect response");
          break;
        }

        response_builder.Reset();

        // read response
        while (!response_builder.Done()) {
          bytes_read = stream_.receive(boost::asio::buffer(buffer));
          response_builder.ProcessInput(buffer.data(), bytes_read);
        }

        session_initializer.ProcessResponse(*response_builder.Get(),
                                            connect_ec);
        if (connect_ec.value() != 0) {
          SSF_LOG("network_proxy", error,
                  "session initializer could not "
                  "process connect response");
          break;
        }
      }

      if (session_initializer.status() ==
              HttpSessionInitializer::Status::kSuccess &&
          connect_ec.value() == 0) {
        return;
      }

      SSF_LOG("network_proxy", error, "connection through proxy failed");
      stream_.close(close_ec);
      ec.assign(ssf::error::broken_pipe, ssf::error::get_ssf_category());
    } catch (const std::exception&) {
      SSF_LOG("network_proxy", error, "connection through proxy failed");
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
 private:
  using Buffer = std::array<char, 4 * 1024>;

 public:
  AsyncHttpConnectOp(Stream& stream, Endpoint* p_local_endpoint,
                     Endpoint peer_endpoint, ConnectHandler handler)
      : coro_(),
        stream_(stream),
        p_local_endpoint_(p_local_endpoint),
        peer_endpoint_(std::move(peer_endpoint)),
        handler_(std::move(handler)),
        p_request_(new std::string()),
        p_buffer_(new Buffer()),
        p_response_builder_(new HttpResponseBuilder()),
        p_session_initializer_(new HttpSessionInitializer()),
        p_http_request_(new HttpRequest()) {}

  AsyncHttpConnectOp(const AsyncHttpConnectOp& other)
      : coro_(other.coro_),
        stream_(other.stream_),
        p_local_endpoint_(other.p_local_endpoint_),
        peer_endpoint_(other.peer_endpoint_),
        handler_(other.handler_),
        p_request_(other.p_request_),
        p_buffer_(other.p_buffer_),
        p_response_builder_(other.p_response_builder_),
        p_session_initializer_(other.p_session_initializer_),
        p_http_request_(other.p_http_request_) {}

  AsyncHttpConnectOp(AsyncHttpConnectOp&& other)
      : coro_(std::move(other.coro_)),
        stream_(other.stream_),
        p_local_endpoint_(other.p_local_endpoint_),
        peer_endpoint_(std::move(other.peer_endpoint_)),
        handler_(std::move(other.handler_)),
        p_request_(std::move(other.p_request_)),
        p_buffer_(std::move(other.p_buffer_)),
        p_response_builder_(std::move(other.p_response_builder_)),
        p_session_initializer_(std::move(other.p_session_initializer_)),
        p_http_request_(std::move(other.p_http_request_)) {}

#include <boost/asio/yield.hpp>
  void operator()(
      const boost::system::error_code& ec = boost::system::error_code(),
      std::size_t size = 0) {
    if (ec) {
      // error
      handler_(ec);
      return;
    }

    boost::system::error_code connect_ec;
    auto& endpoint_context = peer_endpoint_.endpoint_context();

    reenter(coro_) {
      p_session_initializer_->Reset(endpoint_context.remote_host().addr(),
                                    endpoint_context.remote_host().port(),
                                    endpoint_context);

      p_local_endpoint_->set();

      // session initialization (connect request + auth)
      while (p_session_initializer_->status() ==
             HttpSessionInitializer::Status::kContinue) {
        // (re)connect to proxy (new strategy or connection close header)?
        if ((p_session_initializer_->stage() ==
             HttpSessionInitializer::Stage::kConnect) ||
            p_response_builder_->Get()->CloseConnection()) {
          if (stream_.is_open()) {
            stream_.shutdown(boost::asio::socket_base::shutdown_both,
                             close_ec_);
            stream_.close(close_ec_);
          }
          yield stream_.async_connect(
              endpoint_context.http_proxy().ToTcpEndpoint(
                  stream_.get_io_service()),
              std::move(*this));
        }

        // send request
        p_session_initializer_->PopulateRequest(p_http_request_.get(),
                                                connect_ec);
        if (connect_ec.value() != 0) {
          SSF_LOG("network_proxy", error,
                  "session initializer could not "
                  "generate connect request");
          break;
        }

        *p_request_ = p_http_request_->Serialize();

        yield boost::asio::async_write(
            stream_, boost::asio::buffer(*p_request_), std::move(*this));

        // read response
        p_response_builder_->Reset();
        while (!p_response_builder_->Done()) {
          yield stream_.async_receive(boost::asio::buffer(*p_buffer_),
                                      std::move(*this));
          p_response_builder_->ProcessInput(p_buffer_->data(), size);
        }

        p_session_initializer_->ProcessResponse(*p_response_builder_->Get(),
                                                connect_ec);
        if (connect_ec.value() != 0) {
          SSF_LOG("network_proxy", error,
                  "session initializer could not "
                  "process connect response");
          break;
        }
      }

      // initialization finished
      if (p_session_initializer_->status() ==
              HttpSessionInitializer::Status::kSuccess &&
          connect_ec.value() == 0) {
        handler_(close_ec_);
        return;
      }

      SSF_LOG("network_proxy", error, "connection through proxy failed");
      stream_.close(close_ec_);
      close_ec_.assign(ssf::error::broken_pipe, ssf::error::get_ssf_category());
      handler_(close_ec_);
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

  std::shared_ptr<std::string> p_request_;
  std::shared_ptr<Buffer> p_buffer_;
  std::shared_ptr<HttpResponseBuilder> p_response_builder_;
  std::shared_ptr<HttpSessionInitializer> p_session_initializer_;
  std::shared_ptr<HttpRequest> p_http_request_;
  boost::system::error_code close_ec_;
};

template <class Protocol, class Stream, class Endpoint, class ConnectHandler>
inline void* asio_handler_allocate(
    std::size_t size,
    AsyncHttpConnectOp<Protocol, Stream, Endpoint, ConnectHandler>*
        this_handler) {
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
inline bool asio_handler_is_continuation(
    AsyncHttpConnectOp<Protocol, Stream, Endpoint, ConnectHandler>*
        this_handler) {
  return boost_asio_handler_cont_helpers::is_continuation(
      this_handler->handler());
}

template <class Function, class Protocol, class Stream, class Endpoint,
          class ConnectHandler>
inline void asio_handler_invoke(
    Function& function,
    AsyncHttpConnectOp<Protocol, Stream, Endpoint, ConnectHandler>*
        this_handler) {
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

}  // proxy
}  // layer
}  // ssf

#endif  // SSF_LAYER_PROXY_HTTP_CONNECT_OP_H_

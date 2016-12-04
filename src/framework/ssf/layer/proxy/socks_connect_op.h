#ifndef SSF_LAYER_PROXY_SOCKS_CONNECT_OP_H_
#define SSF_LAYER_PROXY_SOCKS_CONNECT_OP_H_

#include <boost/asio/coroutine.hpp>
#include <boost/asio/detail/handler_alloc_helpers.hpp>
#include <boost/asio/detail/handler_cont_helpers.hpp>
#include <boost/asio/detail/handler_invoke_helpers.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/asio/write.hpp>

#include "ssf/layer/proxy/socks_session_initializer.h"

namespace ssf {
namespace layer {
namespace proxy {

template <class Stream, class Endpoint>
class SocksConnectOp {
 public:
  using Buffer = SocksSessionInitializer::Buffer;

 public:
  SocksConnectOp(Stream& stream, Endpoint* p_local_endpoint,
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
      SocksSessionInitializer session_initializer;
      Buffer buffer;
      uint32_t expected_response_size = 0;

      session_initializer.Reset(endpoint_context.remote_host().addr(),
                                endpoint_context.remote_host().port(),
                                endpoint_context, ec);
      if (ec) {
        return;
      }

      stream_.connect(endpoint_context.socks_proxy().ToTcpEndpoint(
                          stream_.get_io_service()),
                      ec);

      p_local_endpoint_->set();
      while (session_initializer.status() ==
             SocksSessionInitializer::Status::kContinue) {
        session_initializer.PopulateRequest(&buffer, &expected_response_size,
                                            connect_ec);
        if (connect_ec.value() != 0) {
          SSF_LOG(kLogError)
              << "network[socks proxy]: session initializer could not "
                 "generate request";
          break;
        }

        if (buffer.size() > 0) {
          boost::asio::write(stream_, boost::asio::buffer(buffer));
        }

        buffer.resize(expected_response_size);

        // read response
        if (buffer.size() > 0) {
          boost::asio::read(stream_, boost::asio::buffer(buffer));

          session_initializer.ProcessResponse(buffer, connect_ec);
        }

        if (connect_ec.value() != 0) {
          SSF_LOG(kLogError)
              << "network[socks proxy]: session initializer could not "
                 "process response";
          break;
        }
      }

      if (session_initializer.status() ==
              SocksSessionInitializer::Status::kSuccess &&
          connect_ec.value() == 0) {
        return;
      }

      SSF_LOG(kLogError)
          << "network[socks proxy]: connection through socks proxy failed";
      stream_.close(close_ec);
      connect_ec.assign(ssf::error::broken_pipe,
                        ssf::error::get_ssf_category());
    } catch (const std::exception& err) {
      SSF_LOG(kLogError)
          << "network[socks proxy]: connection through proxy failed ("
          << err.what() << ")";
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
class AsyncSocksConnectOp {
 private:
  using Buffer = SocksSessionInitializer::Buffer;

 public:
  AsyncSocksConnectOp(Stream& stream, Endpoint* p_local_endpoint,
                      Endpoint peer_endpoint, ConnectHandler handler)
      : coro_(),
        stream_(stream),
        p_local_endpoint_(p_local_endpoint),
        peer_endpoint_(std::move(peer_endpoint)),
        handler_(std::move(handler)),
        p_session_initializer_(new SocksSessionInitializer()),
        p_buffer_(new std::vector<uint8_t>()),
        p_expected_response_size_(new uint32_t(0)) {}

  AsyncSocksConnectOp(const AsyncSocksConnectOp& other)
      : coro_(other.coro_),
        stream_(other.stream_),
        p_local_endpoint_(other.p_local_endpoint_),
        peer_endpoint_(other.peer_endpoint_),
        handler_(other.handler_),
        p_session_initializer_(other.p_session_initializer_),
        p_buffer_(other.p_buffer_),
        p_expected_response_size_(other.p_expected_response_size_) {}

  AsyncSocksConnectOp(AsyncSocksConnectOp&& other)
      : coro_(std::move(other.coro_)),
        stream_(other.stream_),
        p_local_endpoint_(other.p_local_endpoint_),
        peer_endpoint_(std::move(other.peer_endpoint_)),
        handler_(std::move(other.handler_)),
        p_session_initializer_(other.p_session_initializer_),
        p_buffer_(other.p_buffer_),
        p_expected_response_size_(other.p_expected_response_size_) {}

  inline ConnectHandler& handler() const { return handler_; }

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
    boost::system::error_code close_ec;
    auto& endpoint_context = peer_endpoint_.endpoint_context();

    reenter(coro_) {
      p_session_initializer_->Reset(endpoint_context.remote_host().addr(),
                                    endpoint_context.remote_host().port(),
                                    endpoint_context, connect_ec);
      if (connect_ec) {
        // error
        handler_(connect_ec);
        return;
      }

      // connect to socks proxy
      yield stream_.async_connect(endpoint_context.socks_proxy().ToTcpEndpoint(
                                      stream_.get_io_service()),
                                  std::move(*this));

      p_local_endpoint_->set();
      while (p_session_initializer_->status() ==
             SocksSessionInitializer::Status::kContinue) {
        p_session_initializer_->PopulateRequest(
            p_buffer_.get(), p_expected_response_size_.get(), connect_ec);
        if (connect_ec.value() != 0) {
          SSF_LOG(kLogError)
              << "network[socks proxy]: session initializer could not "
                 "generate request";
          break;
        }

        if (p_buffer_->size() > 0) {
          yield boost::asio::async_write(
              stream_, boost::asio::buffer(*p_buffer_), std::move(*this));
        }

        p_buffer_->resize(*p_expected_response_size_);

        // read response
        if (p_buffer_->size() > 0) {
          yield boost::asio::async_read(
              stream_, boost::asio::buffer(*p_buffer_), std::move(*this));

          p_session_initializer_->ProcessResponse(*p_buffer_, connect_ec);
        }

        if (connect_ec.value() != 0) {
          SSF_LOG(kLogError)
              << "network[socks proxy]: session initializer could not "
                 "process response";
          break;
        }
      }

      if (p_session_initializer_->status() ==
              SocksSessionInitializer::Status::kSuccess &&
          connect_ec.value() == 0) {
        handler_(connect_ec);
        return;
      }

      SSF_LOG(kLogError)
          << "network[socks proxy]: connection through socks proxy failed";
      stream_.close(close_ec);
      connect_ec.assign(ssf::error::broken_pipe,
                        ssf::error::get_ssf_category());
      handler_(connect_ec);
    }
  }
#include <boost/asio/unyield.hpp>

 private:
  boost::asio::coroutine coro_;
  Stream& stream_;
  Endpoint* p_local_endpoint_;
  Endpoint peer_endpoint_;
  ConnectHandler handler_;
  std::shared_ptr<SocksSessionInitializer> p_session_initializer_;
  std::shared_ptr<Buffer> p_buffer_;
  std::shared_ptr<uint32_t> p_expected_response_size_;
};

}  // proxy
}  // layer
}  // ssf

#endif  // SSF_LAYER_PROXY_SOCKS_CONNECT_OP_H_
#include <ssf/network/socks/v5/types.h>

#include "ssf/error/error.h"
#include "ssf/layer/proxy/socks5_strategy.h"
#include "ssf/log/log.h"
#include "ssf/utils/enum.h"

#include "ssf/network/socks/v5/request_auth.h"
#include "ssf/network/socks/v5/reply_auth.h"
#include "ssf/network/socks/v5/request.h"
#include "ssf/network/socks/v5/reply.h"

namespace ssf {
namespace layer {
namespace proxy {

Socks5Strategy::Socks5Strategy() : SocksStrategy(State::kAuthenticating) {}

void Socks5Strategy::Init(boost::system::error_code& ec) {
  set_state(State::kAuthenticating);
  connect_reply_.Reset();
}

void Socks5Strategy::PopulateRequest(const std::string& target_host,
                                     uint16_t target_port, Buffer* p_request,
                                     uint32_t* p_expected_response_size,
                                     boost::system::error_code& ec) {
  switch (state()) {
    case State::kAuthenticating:
      GenAuthRequest(p_request, p_expected_response_size, ec);
      break;
    case State::kConnecting:
      GenConnectRequest(target_host, target_port, p_request,
                        p_expected_response_size, ec);
      break;
    case State::kConnected:
      p_request->resize(0);
      *p_expected_response_size = 0;
      break;
    case State::kError:
    default:
      ec.assign(ssf::error::invalid_argument, ssf::error::get_ssf_category());
      break;
  }
}

void Socks5Strategy::ProcessResponse(const Buffer& response,
                                     boost::system::error_code& ec) {
  switch (state()) {
    case State::kAuthenticating:
      ProcessAuthResponse(response, ec);
      break;
    case State::kConnecting:
      ProcessConnectResponse(response, ec);
      break;
    case State::kConnected:
      break;
    case State::kError:
    default:
      ec.assign(ssf::error::invalid_argument, ssf::error::get_ssf_category());
      break;
  }
}

void Socks5Strategy::GenAuthRequest(Buffer* p_request,
                                    uint32_t* p_expected_response_size,
                                    boost::system::error_code& ec) {
  AuthRequest req;
  req.Init({AuthMethod::kNoAuth});

  auto req_buf = req.ConstBuffers();
  p_request->resize(boost::asio::buffer_size(req_buf));
  boost::asio::buffer_copy(boost::asio::buffer(*p_request), req_buf);
  *p_expected_response_size = 2;
}

void Socks5Strategy::ProcessAuthResponse(const Buffer& response,
                                         boost::system::error_code& ec) {
  ReplyAuth auth_reply;
  boost::asio::buffer_copy(auth_reply.MutBuffer(),
                           boost::asio::buffer(response));

  if (auth_reply.auth_method() != ToIntegral(AuthMethod::kNoAuth)) {
    SSF_LOG("network_proxy", error, "SOCKSv5 authentication not supported");
    set_state(State::kError);
    ec.assign(ssf::error::connection_aborted, ssf::error::get_ssf_category());
    return;
  }
  set_state(State::kConnecting);
}

void Socks5Strategy::GenConnectRequest(const std::string& host, uint16_t port,
                                       Buffer* p_request,
                                       uint32_t* p_expected_response_size,
                                       boost::system::error_code& ec) {
  if (connect_reply_.IsAddrTypeSet()) {
    if (connect_reply_.IsComplete()) {
      ec.assign(ssf::error::invalid_argument, ssf::error::get_ssf_category());
      return;
    }
    // no expected request, only fill current connect reply
    auto dyn_buffers = connect_reply_.MutDynamicBuffers();
    p_request->resize(0);
    *p_expected_response_size =
        static_cast<uint32_t>(boost::asio::buffer_size(dyn_buffers));
    return;
  }

  ConnectRequest req;
  req.Init(host, port, ec);
  if (ec) {
    return;
  }

  auto req_buf = req.ConstBuffers();
  p_request->resize(boost::asio::buffer_size(req_buf));
  boost::asio::buffer_copy(boost::asio::buffer(*p_request), req_buf);
  *p_expected_response_size = static_cast<uint32_t>(boost::asio::buffer_size(
      connect_reply_.MutBaseBuffers()));  // response until addr type
}

void Socks5Strategy::ProcessConnectResponse(const Buffer& response,
                                            boost::system::error_code& ec) {
  // connect response size depends on address type
  if (connect_reply_.IsAddrTypeSet() && !connect_reply_.IsComplete()) {
    boost::asio::buffer_copy(connect_reply_.MutDynamicBuffers(),
                             boost::asio::buffer(response));
  } else {
    boost::asio::buffer_copy(connect_reply_.MutBaseBuffers(),
                             boost::asio::buffer(response));
    return;
  }

  if (connect_reply_.IsComplete()) {
    if (connect_reply_.AccessGranted()) {
      SSF_LOG("network_proxy", debug, "SOCKSv5 connected");
      set_state(State::kConnected);
    } else {
      SSF_LOG("network_proxy", error, "SOCKSv5 connection failed");
      set_state(State::kError);
    }
  }
}

}  // proxy
}  // layer
}  // ssf
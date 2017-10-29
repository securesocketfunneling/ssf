#include "ssf/error/error.h"
#include "ssf/log/log.h"
#include "ssf/layer/proxy/socks_session_initializer.h"

namespace ssf {
namespace layer {
namespace proxy {

SocksSessionInitializer::SocksSessionInitializer() {}

void SocksSessionInitializer::Reset(const std::string& target_host,
                                    const std::string& target_port,
                                    const ProxyEndpointContext& proxy_ep_ctx,
                                    boost::system::error_code& ec) {
  status_ = Status::kContinue;
  target_host_ = target_host;
  proxy_ep_ctx_ = proxy_ep_ctx;

  try {
    auto port_conversion = std::stoul(target_port);
    if (port_conversion > 65536) {
      SSF_LOG("network_proxy", error, "SOCKS target port {} out of range",
              target_port);
      ec.assign(ssf::error::invalid_argument, ssf::error::get_ssf_category());
      return;
    }
    target_port_ = static_cast<uint16_t>(port_conversion);
  } catch (const std::exception&) {
    SSF_LOG("network_proxy", error, "cannot process target port {}",
            target_port);
    ec.assign(ssf::error::invalid_argument, ssf::error::get_ssf_category());
    return;
  }

  socks4_strategy_.Init(ec);
  if (ec) {
    SSF_LOG("network_proxy", error, "cannot init SOCKSv4 stategy");
    return;
  }
  socks5_strategy_.Init(ec);
  if (ec) {
    SSF_LOG("network_proxy", error, "cannot init SOCKSv5 stategy");
    return;
  }
}

// socks v4
//   send connect request
//   read response

// socks v5
//   send version and auth request
//   read server selection
//   do auth if any
//   send connect request
//   read response

void SocksSessionInitializer::PopulateRequest(
    Buffer* p_request, uint32_t* p_expected_response_size,
    boost::system::error_code& ec) {
  if (proxy_ep_ctx_.socks_proxy().IsVersion4()) {
    socks4_strategy_.PopulateRequest(target_host_, target_port_, p_request,
                                     p_expected_response_size, ec);
  } else if (proxy_ep_ctx_.socks_proxy().IsVersion5()) {
    socks5_strategy_.PopulateRequest(target_host_, target_port_, p_request,
                                     p_expected_response_size, ec);
  } else {
    SSF_LOG("network_proxy", error, "invalid SOCKS version {}",
            proxy_ep_ctx_.socks_proxy().version);
    ec.assign(ssf::error::invalid_argument, ssf::error::get_ssf_category());
    return;
  }
}

void SocksSessionInitializer::ProcessResponse(const Buffer& response,
                                              boost::system::error_code& ec) {
  if (proxy_ep_ctx_.socks_proxy().IsVersion4()) {
    socks4_strategy_.ProcessResponse(response, ec);
    status_ = ((socks4_strategy_.state() == Socks4Strategy::State::kConnected)
                   ? Status::kSuccess
                   : Status::kError);
  } else if (proxy_ep_ctx_.socks_proxy().IsVersion5()) {
    socks5_strategy_.ProcessResponse(response, ec);
    switch (socks5_strategy_.state()) {
      case Socks5Strategy::State::kConnected:
        status_ = Status::kSuccess;
        return;
      case Socks5Strategy::State::kError:
        status_ = Status::kError;
        return;
      default:
        status_ = Status::kContinue;
        return;
    }
  } else {
    SSF_LOG("network_proxy", error, "invalid SOCKS version {}",
            proxy_ep_ctx_.socks_proxy().version);
    ec.assign(ssf::error::invalid_argument, ssf::error::get_ssf_category());
    return;
  }
}

}  // ssf
}  // layer
}  // ssf
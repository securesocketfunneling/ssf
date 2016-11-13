#ifndef SSF_LAYER_PROXY_SOCKS5_STRATEGY_H_
#define SSF_LAYER_PROXY_SOCKS5_STRATEGY_H_

#include "ssf/layer/proxy/socks_strategy.h"

#include "ssf/network/socks/v5/request_auth.h"
#include "ssf/network/socks/v5/reply_auth.h"
#include "ssf/network/socks/v5/request.h"
#include "ssf/network/socks/v5/reply.h"

namespace ssf {
namespace layer {
namespace proxy {

class Socks5Strategy : public SocksStrategy {
 private:
  using AuthRequest = ssf::network::socks::v5::RequestAuth;
  using AuthReply = ssf::network::socks::v5::AuthReply;
  using ConnectRequest = ssf::network::socks::v5::Request;
  using ConnectReply = ssf::network::socks::v5::Reply;

 public:
  Socks5Strategy();

  void Init(boost::system::error_code& ec) override;

  void PopulateRequest(const std::string& host, uint16_t port,
                       Buffer* p_request, uint32_t* p_expected_response_size,
                       boost::system::error_code& ec) override;
  void ProcessResponse(const Buffer& response,
                       boost::system::error_code& ec) override;

 private:
  void GenAuthRequest(Buffer* p_request, uint32_t* p_expected_response_size,
                      boost::system::error_code& ec);

  void ProcessAuthResponse(const Buffer& response,
                           boost::system::error_code& ec);

  void GenConnectRequest(const std::string& host, uint16_t port,
                         Buffer* p_request, uint32_t* p_expected_response_size,
                         boost::system::error_code& ec);

  void ProcessConnectResponse(const Buffer& response,
                              boost::system::error_code& ec);

 private:
  ConnectReply connect_reply_;
};

}  // proxy
}  // layer
}  // ssf

#endif  // SSF_LAYER_PROXY_SOCKS5_STRATEGY_H_
#ifndef SSF_LAYER_PROXY_SOCKS4_STRATEGY_H_
#define SSF_LAYER_PROXY_SOCKS4_STRATEGY_H_

#include "ssf/layer/proxy/socks_strategy.h"

#include "ssf/network/socks/v4/request.h"
#include "ssf/network/socks/v4/reply.h"

namespace ssf {
namespace layer {
namespace proxy {

class Socks4Strategy : public SocksStrategy {
 private:
  using Request = ssf::network::socks::v4::Request;
  using Reply = ssf::network::socks::v4::Reply;

 public:
  Socks4Strategy();

  void Init(boost::system::error_code& ec) override;

  void PopulateRequest(const std::string& host, uint16_t port,
                       Buffer* p_request, uint32_t* p_expected_response_size,
                       boost::system::error_code& ec) override;
  void ProcessResponse(const Buffer& response,
                       boost::system::error_code& ec) override;
};

}  // proxy
}  // layer
}  // ssf

#endif  // SSF_LAYER_PROXY_SOCKS4_STRATEGY_H_
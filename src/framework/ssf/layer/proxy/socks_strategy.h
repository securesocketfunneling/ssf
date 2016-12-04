#ifndef SSF_LAYER_PROXY_SOCKS_STRATEGY_H_
#define SSF_LAYER_PROXY_SOCKS_STRATEGY_H_

#include <cstdint>
#include <vector>

#include "ssf/layer/proxy/proxy_endpoint_context.h"

namespace ssf {
namespace layer {
namespace proxy {

class SocksStrategy {
 public:
  enum State {
    kError = -1,
    kAuthenticating = 0,
    kConnecting = 1,
    kConnected = 2
  };

  using Buffer = std::vector<uint8_t>;

 public:
  virtual ~SocksStrategy() {}

  virtual void Init(boost::system::error_code& ec) = 0;

  // @param p_expected_response_size response size in bytes expected from this
  //   request
  virtual void PopulateRequest(const std::string& host, uint16_t port,
                               Buffer* p_request,
                               uint32_t* p_expected_response_size,
                               boost::system::error_code& ec) = 0;

  virtual void ProcessResponse(const Buffer& response,
                               boost::system::error_code& ec) = 0;

  State state() const { return state_; };
  void set_state(State state) { state_ = state; }

 protected:
  SocksStrategy(State state) : state_(state) {}

 private:
  State state_;
};

}  // proxy
}  // layer
}  // ssf

#endif  // SSF_LAYER_PROXY_SOCKS_STRATEGY_H_
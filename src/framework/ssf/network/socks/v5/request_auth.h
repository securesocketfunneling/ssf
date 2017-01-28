#ifndef SSF_NETWORK_SOCKS_V5_REQUEST_AUTH_H_
#define SSF_NETWORK_SOCKS_V5_REQUEST_AUTH_H_

#include <cstdint>
#include <array>
#include <vector>

#include <boost/asio/buffer.hpp>

#include "ssf/network/socks/v5/types.h"

namespace ssf {
namespace network {
namespace socks {
namespace v5 {

class RequestAuth {
 public:
  RequestAuth();

  void Init(const std::vector<AuthMethod>& methods);

  uint8_t auth_supported_count() const { return auth_supported_count_; }

  void AddAuthMethod(uint8_t auth_method);

  boost::asio::mutable_buffers_1 MutAuthSupportedBuffers();
  boost::asio::mutable_buffers_1 MutAuthBuffers();
  std::vector<boost::asio::const_buffer> ConstBuffers();

  bool IsNoAuthPresent();

 private:
  uint8_t version_;
  uint8_t auth_supported_count_;
  std::vector<uint8_t> auth_methods_;
};

}  // v5
}  // socks
}  // network
}  // ssf

#endif  // SSF_NETWORK_SOCKS_V5_REQUEST_AUTH_H_

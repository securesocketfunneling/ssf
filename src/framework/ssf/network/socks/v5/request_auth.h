#ifndef SSF_NETWORK_SOCKS_V5_REQUEST_AUTH_H_
#define SSF_NETWORK_SOCKS_V5_REQUEST_AUTH_H_

#include <cstdint>
#include <array>
#include <vector>

#include <boost/asio/buffer.hpp>

namespace ssf {
namespace network {
namespace socks {
namespace v5 {

class RequestAuth {
 public:
  // Authentication method constants
  enum class AuthMethod : uint8_t {
    kNoAuth = 0x00,
    kGSSAPI = 0x01,
    kUserPassword = 0x02,
    kError = 0xFF
  };

 public:
  void Init(AuthMethod method);

  uint8_t auth_supported_count() const { return auth_supported_count_; }

  void AddAuthMethod(uint8_t authMethod);

  std::array<boost::asio::mutable_buffer, 1> MutBuffers();
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

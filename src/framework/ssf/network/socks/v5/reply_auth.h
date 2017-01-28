#ifndef SSF_NETWORK_SOCKS_V5_REPLY_AUTH_H_
#define SSF_NETWORK_SOCKS_V5_REPLY_AUTH_H_

#include <cstdint>
#include <array>

#include <boost/asio/buffer.hpp>

#include "ssf/network/socks/v5/request_auth.h"
#include "ssf/network/socks/v5/types.h"

namespace ssf {
namespace network {
namespace socks {
namespace v5 {

class ReplyAuth {
 public:
  ReplyAuth();
  ReplyAuth(AuthMethod auth_method);

  uint8_t auth_method() const { return auth_method_; }

  std::array<boost::asio::const_buffer, 2> ConstBuffer() const;
  std::array<boost::asio::mutable_buffer, 2> MutBuffer();

 private:
  uint8_t version_;
  uint8_t auth_method_;
};

}  // v5
}  // socks
}  // network
}  // ssf

#endif  // SSF_NETWORK_SOCKS_V5_REPLY_AUTH_H_

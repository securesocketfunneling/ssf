#include "ssf/network/socks/v5/reply_auth.h"

#include "ssf/network/socks/socks.h"

#include "ssf/utils/enum.h"

namespace ssf {
namespace network {
namespace socks {
namespace v5 {

ReplyAuth::ReplyAuth() {}

ReplyAuth::ReplyAuth(AuthMethod auth_method)
    : version_(ToIntegral(Socks::Version::kV5)),
      auth_method_(ToIntegral(auth_method)) {}

std::array<boost::asio::const_buffer, 2> ReplyAuth::ConstBuffer() const {
  return {{
      boost::asio::buffer(&version_, 1), boost::asio::buffer(&auth_method_, 1)
  }};
}

std::array<boost::asio::mutable_buffer, 2> ReplyAuth::MutBuffer() {
  return {{boost::asio::buffer(&version_, 1),
           boost::asio::buffer(&auth_method_, 1)}};
}

}  // v5
}  // socks
}  // network
}  // ssf

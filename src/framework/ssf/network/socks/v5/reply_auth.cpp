#include "ssf/network/socks/v5/reply_auth.h"

#include "ssf/network/socks/socks.h"

#include "ssf/utils/enum.h"

namespace ssf {
namespace network {
namespace socks {
namespace v5 {

AuthReply::AuthReply() {}

AuthReply::AuthReply(uint8_t authMethod)
    : version_(ToIntegral(Socks::Version::kV5)), auth_method_(authMethod) {}

std::array<boost::asio::const_buffer, 2> AuthReply::ConstBuffer() const {
  std::array<boost::asio::const_buffer, 2> buf = {{
      boost::asio::buffer(&version_, 1), boost::asio::buffer(&auth_method_, 1),
  }};
  return buf;
}

std::array<boost::asio::mutable_buffer, 2> AuthReply::MutBuffer() {
  std::array<boost::asio::mutable_buffer, 2> buf = {{
      boost::asio::buffer(&version_, 1), boost::asio::buffer(&auth_method_, 1),
  }};
  return buf;
}

}  // v5
}  // socks
}  // network
}  // ssf

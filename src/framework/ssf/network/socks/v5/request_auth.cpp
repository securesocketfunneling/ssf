#include "ssf/network/socks/v5/request_auth.h"

#include "ssf/network/socks/socks.h"

#include "ssf/utils/enum.h"

namespace ssf {
namespace network {
namespace socks {
namespace v5 {

void RequestAuth::Init(AuthMethod method) {
  version_ = ToIntegral(Socks::Version::kV5);
  auth_supported_count_ = 1;
  auth_methods_.push_back(ToIntegral(method));
}

void RequestAuth::AddAuthMethod(uint8_t auth_method) {
  auth_methods_.push_back(auth_method);
}

std::array<boost::asio::mutable_buffer, 1> RequestAuth::MutBuffers() {
  std::array<boost::asio::mutable_buffer, 1> buf = {{
      boost::asio::buffer(&auth_supported_count_, 1),
  }};
  return buf;
}

std::vector<boost::asio::const_buffer> RequestAuth::ConstBuffers() {
  std::vector<boost::asio::const_buffer> buf{
      {boost::asio::buffer(&version_, 1),
       boost::asio::buffer(&auth_supported_count_, 1)}};

  buf.push_back(boost::asio::buffer(auth_methods_));

  return buf;
}

bool RequestAuth::IsNoAuthPresent() {
  for (const auto& auth_method : auth_methods_) {
    if (auth_method == ToIntegral(AuthMethod::kNoAuth)) {
      return true;
    }
  }

  return false;
}

}  // v5
}  // socks
}  // network
}  // ssf

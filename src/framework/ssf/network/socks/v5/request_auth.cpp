#include "ssf/network/socks/v5/request_auth.h"

#include "ssf/network/socks/socks.h"

#include "ssf/utils/enum.h"

namespace ssf {
namespace network {
namespace socks {
namespace v5 {

RequestAuth::RequestAuth()
    : version_(ToIntegral(Socks::Version::kV5)), auth_supported_count_(0) {}

void RequestAuth::Init(const std::vector<AuthMethod>& methods) {
  version_ = ToIntegral(Socks::Version::kV5);
  auth_supported_count_ = static_cast<uint8_t>(methods.size());
  for (const auto& method : methods) {
    auth_methods_.push_back(ToIntegral(method));
  }
}

void RequestAuth::AddAuthMethod(uint8_t auth_method) {
  auth_methods_.push_back(auth_method);
}

boost::asio::mutable_buffers_1 RequestAuth::MutAuthSupportedBuffers() {
  return boost::asio::mutable_buffers_1(
      boost::asio::buffer(&auth_supported_count_, 1));
}

boost::asio::mutable_buffers_1 RequestAuth::MutAuthBuffers() {
  if (auth_methods_.size() != auth_supported_count_) {
    auth_methods_.resize(auth_supported_count_);
  }

  return boost::asio::mutable_buffers_1(boost::asio::buffer(auth_methods_));
}

std::vector<boost::asio::const_buffer> RequestAuth::ConstBuffers() {
  return {{boost::asio::buffer(&version_, 1),
           boost::asio::buffer(&auth_supported_count_, 1),
           boost::asio::buffer(auth_methods_)}};
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

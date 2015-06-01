#include "services/socks/v5/request_auth.h"

#include <string>  // NOLINT
#include <iostream>  // NOLINT

#include <boost/asio.hpp>  // NOLINT
#include <boost/asio/coroutine.hpp>  // NOLINT


namespace ssf { namespace socks { namespace v5 {
//-----------------------------------------------------------------------------
uint8_t RequestAuth::numberOfAuthSupported() const {
  return numberOfAuthSupported_;
}

//-----------------------------------------------------------------------------
void RequestAuth::addAuthMethod(uint8_t authMethod) {
  authMethods_.push_back(authMethod);
}

//-----------------------------------------------------------------------------
std::array<boost::asio::mutable_buffer, 1> RequestAuth::Buffers() {
  std::array<boost::asio::mutable_buffer, 1> buf = {
    {
      boost::asio::buffer(&numberOfAuthSupported_, 1),
    }
  };
  return buf;
}

//----------------------------------------------------------------------------
bool RequestAuth::is_no_auth_present() {
  for (size_t i = 0; i < numberOfAuthSupported_; ++i) {
    if (authMethods_[i] == kNoAuth) {
      return true;
    }
  }

  return false;
}

}  // v5
}  // socks
}  // ssf


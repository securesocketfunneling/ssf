#include "services/socks/v5/reply_auth.h"

#include <iostream>  // NOLINT

namespace ssf { namespace socks { namespace v5 {
//-----------------------------------------------------------------------------
  AuthReply::AuthReply(uint8_t authMethod)
  : version_(0x05),
    authMethod_(authMethod) { }


//-----------------------------------------------------------------------------
  std::array<boost::asio::const_buffer, 2> AuthReply::Buffer() const {
  std::array<boost::asio::const_buffer, 2> buf = {
    {
      boost::asio::buffer(&version_, 1),
      boost::asio::buffer(&authMethod_, 1),
    }
  };
  return buf;
}

}  // v5
}  // socks
}  // ssf

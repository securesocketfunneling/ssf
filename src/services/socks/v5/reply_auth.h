#ifndef SSF_V5_REPLY_AUTH_H_
#define SSF_V5_REPLY_AUTH_H_

#include <cstdint>
#include <array>

#include <boost/asio/write.hpp>
#include <boost/asio/buffer.hpp>

namespace ssf { namespace socks { namespace v5 {
//-----------------------------------------------------------------------------

class AuthReply {
 public:
  AuthReply(uint8_t authMethod);

  std::array<boost::asio::const_buffer, 2> Buffer() const;

 private:
  uint8_t version_;
  uint8_t authMethod_;
};


//-----------------------------------------------------------------------------
//  S E N D   R E P L Y
//-----------------------------------------------------------------------------
template<class VerifyHandler, class StreamSocket>
void AsyncSendAuthReply(StreamSocket& c, const AuthReply& r, VerifyHandler handler) {
  boost::asio::async_write(c, r.Buffer(), handler);
}

//-----------------------------------------------------------------------------
}  // v5
}  // socks
}  // ssf

#endif  // SSF_V5_REPLY_AUTH_H_

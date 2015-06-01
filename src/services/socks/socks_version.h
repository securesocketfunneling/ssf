#ifndef SSF_SOCKS_VERSION_H_
#define SSF_SOCKS_VERSION_H_

#include <cstdint>
#include <array>
#include <boost/asio/buffer.hpp>

namespace ssf { namespace services { namespace socks {
//-----------------------------------------------------------------------------

class Version {
 public:
  uint8_t Number() const { return version_number_; }

  std::array<boost::asio::mutable_buffer, 1> Buffer() {
    std::array<boost::asio::mutable_buffer, 1> buf = {
      {
        boost::asio::buffer(&version_number_, 1)
      }
    };
    return buf;
  }

 private:
  uint8_t version_number_;
};

}  // socks
}  // services
}  // ssf


#endif  // SSF_SOCKS_VERSION_H_

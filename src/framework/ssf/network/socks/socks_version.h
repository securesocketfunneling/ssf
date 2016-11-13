#ifndef SSF_NETWORK_SOCKS_VERSION_H_
#define SSF_NETWORK_SOCKS_VERSION_H_

#include <cstdint>
#include <array>
#include <boost/asio/buffer.hpp>

namespace ssf {
namespace network {
namespace socks {

class Version {
 public:
  uint8_t version() const { return version_; }

  std::array<boost::asio::mutable_buffer, 1> Buffer() {
    std::array<boost::asio::mutable_buffer, 1> buf = {
        {boost::asio::buffer(&version_, 1)}};
    return buf;
  }

 private:
  uint8_t version_;
};

}  // socks
}  // network
}  // ssf

#endif  // SSF_NETWORK_SOCKS_VERSION_H_

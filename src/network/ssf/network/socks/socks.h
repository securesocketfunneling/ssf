#ifndef SSF_NETWORK_SOCKS_H_
#define SSF_NETWORK_SOCKS_H_

#include <cstdint>

namespace ssf {
namespace network {

struct Socks {
  enum class Version : uint8_t { kVUnknown = 0, kV4 = 0x04, kV5 = 0x05 };
};

}  // network
}  // ssf

#endif  // SSF_NETWORK_SOCKS_H_

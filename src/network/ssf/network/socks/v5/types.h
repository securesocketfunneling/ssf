#ifndef SSF_NETWORK_SOCKS_V5_TYPES_H_
#define SSF_NETWORK_SOCKS_V5_TYPES_H_

#include <cstdint>

namespace ssf {
namespace network {
namespace socks {
namespace v5 {

// Authentication method constants
enum class AuthMethod : uint8_t {
  kNoAuth = 0x00,
  kGSSAPI = 0x01,
  kUserPassword = 0x02,
  kUnsupportedAuth = 0xFF
};

// Command types constants
enum class CommandType : uint8_t { kConnect = 0x01, kBind = 0x02, kUDP = 0x03 };

// Address type constants
enum class AddressType : uint8_t { kIPv4 = 0x01, kDNS = 0x03, kIPv6 = 0x04 };

// Command status constants
enum class CommandStatus : uint8_t {
  kSucceeded = 0x00,
  kGeneralServerFailure = 0x01,
  kConnectionNotAllowed = 0x02,
  kNetworkUnreachable = 0x03,
  kHostUnreachable = 0x04,
  kConnectionRefused = 0x05,
  kTTLExpired = 0x06,
  kCommandNotSupported = 0x07,
  kAddressTypeNotSupported = 0x08
};

}  // v5
}  // socks
}  // network
}  // ssf

#endif  // SSF_NETWORK_SOCKS_V5_TYPES_H_

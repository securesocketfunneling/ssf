#ifndef SSF_LAYER_PROXY_BASE64_H_
#define SSF_LAYER_PROXY_BASE64_H_

#include <cstdint>

#include <string>
#include <vector>

namespace ssf {
namespace layer {
namespace proxy {
namespace detail {

class Base64 {
 public:
  using Buffer = std::vector<uint8_t>;

 public:
  static std::string Encode(const std::string& input);
  static std::string Encode(const Buffer& input);
  static Buffer Decode(const std::string& input);
};

}  // detail
}  // proxy
}  // layer
}  // ssf

#endif  // SSF_LAYER_PROXY_BASE64_H_
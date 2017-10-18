#ifndef SSF_COMMON_UTILS_TO_UNDERLYING_H_
#define SSF_COMMON_UTILS_TO_UNDERLYING_H_

#include <type_traits>

namespace ssf {

template <typename Enum>
constexpr typename std::underlying_type_t<Enum> to_underlying(const Enum& value) {
  return static_cast<std::underlying_type_t<Enum>>(value);
}

}  // ssf

#endif  // SSF_COMMON_UTILS_TO_UNDERLYING_H_


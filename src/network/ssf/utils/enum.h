#ifndef SSF_UTILS_ENUM_H_
#define SSF_UTILS_ENUM_H_

#include <type_traits>

namespace ssf {

template <class Enum>
auto ToIntegral(Enum value) ->
    typename std::underlying_type<Enum>::type {
  return static_cast<typename std::underlying_type<Enum>::type>(value);
}

}  // ssf

#endif  // SSF_UTILS_ENUM_H_
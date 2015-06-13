#ifndef SSF_COMMON_UTILS_MAP_HELPERS_H_
#define SSF_COMMON_UTILS_MAP_HELPERS_H_

#include <map>

namespace helpers {

template <class Key, class Value>
Value GetField(const Key field, const std::map<Key, Value>& parameters) {
    auto it = parameters.find(field);

    if (it == std::end(parameters)) {
      return Value();
    }

    return it->second;
  }

}  // helpers

#endif  // SSF_COMMON_UTILS_MAP_HELPERS_H_
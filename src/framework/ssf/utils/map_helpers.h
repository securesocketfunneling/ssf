#ifndef SSF_UTILS_MAP_HELPERS_H_
#define SSF_UTILS_MAP_HELPERS_H_

#include <map>

namespace ssf {
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
}  // ssf

#endif  // SSF_UTILS_MAP_HELPERS_H_

#include "ssf/layer/parameters.h"

#include <msgpack.hpp>
#include <sstream>

namespace ssf {
namespace layer {

std::string serialize_parameter_stack(const ParameterStack &stack) {
  std::ostringstream ostrs;

  msgpack::pack(ostrs, stack);

  return ostrs.str();
}

ParameterStack unserialize_parameter_stack(
    const std::string &serialized_stack) {
  try {
    auto obj_handle =
        msgpack::unpack(serialized_stack.data(), serialized_stack.size());
    auto obj = obj_handle.get();

    return obj.as<ParameterStack>();
  } catch (const std::exception &) {
    return ParameterStack();
  }
}

}  // layer
}  // ssf

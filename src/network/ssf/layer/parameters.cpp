#include "ssf/layer/parameters.h"

#include <msgpack.hpp>


namespace ssf {
namespace layer {

std::string serialize_parameter_stack(
    const ParameterStack &stack) {
  std::ostringstream ostrs;

  msgpack::pack(ostrs, stack);

  return ostrs.str();
}

ParameterStack unserialize_parameter_stack(
    const std::string& serialized_stack) {
  try {
    auto obj_handle = msgpack::unpack(serialized_stack.data(), serialized_stack.size());
    auto obj = obj_handle.get();

    return obj.as<ParameterStack>();
  } catch (const std::exception &) {
    return ParameterStack();
  }
}

void ptree_entry_to_query(const boost::property_tree::ptree& ptree,
                         const std::string& entry_name,
                         LayerParameters* p_params) {
  auto given_entry = ptree.get_child_optional(entry_name);
  if (given_entry) {
    (*p_params)[entry_name] = given_entry.get().data();
  }
}

}  // layer
}  // ssf

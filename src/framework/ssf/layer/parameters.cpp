#include "ssf/layer/parameters.h"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <boost/serialization/list.hpp>
#include <boost/serialization/map.hpp>

namespace ssf {
namespace layer {

std::string serialize_parameter_stack(
    const ParameterStack &stack) {
  std::ostringstream ostrs;
  boost::archive::text_oarchive ar(ostrs);

  ar << stack;

  return ostrs.str();
}

ParameterStack unserialize_parameter_stack(
    const std::string& serialized_stack) {
  try {
    std::istringstream istrs(serialized_stack);
    boost::archive::text_iarchive ar(istrs);
    ParameterStack deserialized;

    ar >> deserialized;

    return deserialized;
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

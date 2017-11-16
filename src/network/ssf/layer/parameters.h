#ifndef SSF_LAYER_PARAMETERS_H_
#define SSF_LAYER_PARAMETERS_H_

#include <list>
#include <map>
#include <string>

#include <boost/property_tree/ptree.hpp>

namespace ssf {
namespace layer {

using LayerParameters = std::map<std::string, std::string>;
using ParameterStack = std::list<LayerParameters>;

std::string serialize_parameter_stack(const ParameterStack& stack);

ParameterStack unserialize_parameter_stack(const std::string& serialized_stack);

void ptree_entry_to_query(const boost::property_tree::ptree& ptree,
                          const std::string& entry_name,
                          LayerParameters* p_params);

}  // layer
}  // ssf

#endif  // SSF_LAYER_PARAMETERS_H_

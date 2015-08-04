#ifndef SSF_CORE_VIRTUAL_NETWORK_PARAMETERS_H_
#define SSF_CORE_VIRTUAL_NETWORK_PARAMETERS_H_

#include <list>
#include <map>
#include <string>

#include <boost/property_tree/ptree.hpp>

namespace virtual_network {

typedef std::map<std::string, std::string> LayerParameters;
typedef std::list<LayerParameters> ParameterStack;

inline void ptree_entry_to_query(const boost::property_tree::ptree& ptree,
                         const std::string& entry_name,
                         LayerParameters* p_params) {
  auto given_entry = ptree.get_child_optional(entry_name);
  if (given_entry) {
    (*p_params)[entry_name] = given_entry.get().data();
  }
}

}  // virtual_network

#endif  // SSF_CORE_VIRTUAL_NETWORK_PARAMETERS_H_

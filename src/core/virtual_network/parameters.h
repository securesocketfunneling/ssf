#ifndef SSF_CORE_VIRTUAL_NETWORK_PARAMETERS_H_
#define SSF_CORE_VIRTUAL_NETWORK_PARAMETERS_H_

#include <string>
#include <map>
#include <list>

namespace virtual_network {

typedef std::map<std::string, std::string> LayerParameters;
typedef std::list<LayerParameters> ParameterStack;

}  // virtual_network

#endif  // SSF_CORE_VIRTUAL_NETWORK_PARAMETERS_H_

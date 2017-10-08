#ifndef SSF_SERVICES_USER_SERVICES_PARAMETERS_H_
#define SSF_SERVICES_USER_SERVICES_PARAMETERS_H_

#include <map>
#include <string>
#include <vector>

namespace ssf {

using UserServiceParameterBag = std::map<std::string, std::string>;
using UserServiceParameters =
    std::map<std::string, std::vector<UserServiceParameterBag>>;

}  // ssf

#endif  // SSF_SERVICES_USER_SERVICES_PARAMETERS_H_

#ifndef SSF_CORE_CLIENT_QUERY_FACTORY_H_
#define SSF_CORE_CLIENT_QUERY_FACTORY_H_

#include <list>
#include <string>

#include <ssf/layer/parameters.h>

#include "common/config/config.h"

namespace ssf {

using CircuitBouncers = std::list<std::string>;
using NetworkQuery = ssf::layer::ParameterStack;

NetworkQuery GenerateTCPNetworkQuery(const std::string& remote_addr,
                                     const std::string& remote_port,
                                     const CircuitBouncers& nodes);

NetworkQuery GenerateTLSNetworkQuery(const std::string& remote_addr,
                                     const std::string& remote_port,
                                     const ssf::Config& ssf_config,
                                     const CircuitBouncers& nodes);
}

#endif  // SSF_CORE_CLIENT_QUERY_FACTORY_H_

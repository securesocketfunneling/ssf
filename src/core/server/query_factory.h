#ifndef SSF_CORE_SERVER_QUERY_FACTORY_H_
#define SSF_CORE_SERVER_QUERY_FACTORY_H_

#include <list>
#include <string>

#include <ssf/layer/parameters.h>

#include "common/config/config.h"

namespace ssf {

using NetworkQuery = ssf::layer::ParameterStack;

NetworkQuery GenerateServerTCPNetworkQuery(const std::string& remote_port);

NetworkQuery GenerateServerTLSNetworkQuery(const std::string& remote_port,
                                           const ssf::Config& ssf_config);
}

#endif  // SSF_CORE_SERVER_QUERY_FACTORY_H_

#ifndef SSF_CORE_CLIENT_CLIENT_HELPER_H_
#define SSF_CORE_CLIENT_CLIENT_HELPER_H_

#include <string>

#include "common/config/config.h"
#include "core/network_protocol.h"

namespace ssf {

using NetworkProtocol = ssf::network::NetworkProtocol;

NetworkProtocol::Query GenerateNetworkQuery(const std::string& remote_addr,
                                            const std::string& remote_port,
                                            const ssf::config::Config& config);

}  // ssf

#endif  // SSF_CORE_CLIENT_CLIENT_HELPER_H_

#include "core/client/client_helper.h"

namespace ssf {

NetworkProtocol::Query GenerateNetworkQuery(
    const std::string& remote_addr, const std::string& remote_port,
    const ssf::config::Config& ssf_config) {
  // put remote_addr and remote_port as last node in the circuit
  std::string first_node_addr;
  std::string first_node_port;
  ssf::config::NodeList nodes = ssf_config.circuit().nodes();
  if (nodes.size()) {
    auto first_node = nodes.front();
    nodes.pop_front();
    first_node_addr = first_node.addr();
    first_node_port = first_node.port();
    nodes.emplace_back(remote_addr, remote_port);
  } else {
    first_node_addr = remote_addr;
    first_node_port = remote_port;
  }

  return NetworkProtocol::GenerateClientQuery(first_node_addr, first_node_port,
                                              ssf_config, nodes);
}

}  // ssf

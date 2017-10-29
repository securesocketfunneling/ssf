#include "common/config/circuit.h"

#include <boost/algorithm/string.hpp>

#include <ssf/log/log.h>

namespace ssf {
namespace config {

CircuitNode::CircuitNode(const std::string& addr, const std::string& port)
    : addr_(addr), port_(port) {}

Circuit::Circuit() : nodes_() {}

void Circuit::Update(const PTree& pt) {
  for (const auto& child : pt) {
    auto opt_host = child.second.get_child_optional("host");
    auto opt_port = child.second.get_child_optional("port");
    if (opt_host && opt_port) {
      std::string host(opt_host.get().data());
      std::string port(opt_port.get().data());
      boost::trim(host);
      boost::trim(port);
      nodes_.emplace_back(host, port);
    }
  }
}

void Circuit::Log() const {
  if (nodes_.size() == 0) {
    SSF_LOG(kLogInfo) << "[config][circuit] <None>";
    return;
  }

  unsigned int i = 0;
  for (const auto& node : nodes_) {
    ++i;
    SSF_LOG(kLogInfo) << "[config][circuit] " << std::to_string(i) << ". <"
                      << node.addr() << ":" << node.port() << ">";
  }
}

}  // config
}  // ssf

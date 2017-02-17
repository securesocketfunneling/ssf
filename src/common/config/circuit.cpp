#include "common/config/circuit.h"

#include <ssf/log/log.h>

namespace ssf {
namespace config {

CircuitNode::CircuitNode(const std::string& addr, const std::string& port)
    : addr_(addr), port_(port) {}

Circuit::Circuit() : nodes_() {}

void Circuit::Update(const PTree& pt) {
  for (const auto& child : pt) {
    auto host = child.second.get_child_optional("host");
    auto port = child.second.get_child_optional("port");
    if (host && port) {
      nodes_.emplace_back(host.get().data(), port.get().data());
    }
  }
}

void Circuit::Log() const {
  if (nodes_.size() == 0) {
    SSF_LOG(kLogInfo) << "config[circuit]: <None>";
    return;
  }

  unsigned int i = 0;
  for (const auto& node : nodes_) {
    ++i;
    SSF_LOG(kLogInfo) << "config[circuit]: " << std::to_string(i) << ". <"
                      << node.addr() << ":" << node.port() << ">";
  }
}

}  // config
}  // ssf

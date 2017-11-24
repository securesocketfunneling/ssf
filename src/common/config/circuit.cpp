#include "common/config/circuit.h"

#include <boost/algorithm/string.hpp>

#include <ssf/log/log.h>

namespace ssf {
namespace config {

CircuitNode::CircuitNode(const std::string& addr, const std::string& port)
    : addr_(addr), port_(port) {}

Circuit::Circuit() : nodes_() {}

void Circuit::Update(const Json& json) {
  for (const auto& child : json) {
    if (child.count("host") == 1 && child.count("port") == 1) {
      std::string host(child.at("host").get<std::string>());
      std::string port(child.at("port").get<std::string>());
      boost::trim(host);
      boost::trim(port);
      nodes_.emplace_back(host, port);
    }
  }
}

void Circuit::Log() const {
  if (nodes_.size() == 0) {
    SSF_LOG("config", info, "[circuit] <None>");
    return;
  }

  unsigned int i = 0;
  for (const auto& node : nodes_) {
    ++i;
    SSF_LOG("config", info, "[circuit] {}. <{}:{}>", std::to_string(i),
            node.addr(), node.port());
  }
}

}  // config
}  // ssf

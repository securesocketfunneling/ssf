#include "core/circuit/config.h"

#include <fstream>

#include <ssf/log/log.h>

#include "common/error/error.h"

namespace ssf {
namespace circuit {

CircuitNode::CircuitNode(const std::string& addr, const std::string& port)
    : addr_(addr), port_(port) {}

Config::Config() : nodes_() {}

void Config::Update(const std::string& filepath,
                    boost::system::error_code& ec) {
  std::string conf_file("circuit.txt");
  if (filepath == "") {
    std::ifstream ifile(conf_file);
    if (!ifile.good()) {
      return;
    }
    ifile.close();
  } else {
    conf_file = filepath;
  }

  SSF_LOG(kLogInfo) << "config[circuit]: loading file <" << conf_file << ">";

  std::ifstream file(conf_file);

  if (!file.is_open()) {
    SSF_LOG(kLogError) << "config[circuit]: could not open file <" << conf_file
                       << ">";
    ec.assign(::error::invalid_argument, ::error::get_ssf_category());
    return;
  }

  std::string line;

  while (std::getline(file, line)) {
    size_t position = line.find(":");
    if (position == std::string::npos) {
      SSF_LOG(kLogError) << "config[circuit]: invalid line " << line;
      ec.assign(::error::invalid_argument, ::error::get_ssf_category());
      break;
    }
    nodes_.emplace_back(line.substr(0, position), line.substr(position + 1));
  }

  file.close();
}

void Config::Log() const {
  for (auto& node : nodes_) {
    SSF_LOG(kLogInfo) << "config[circuit]: <" << node.addr() << ":"
                      << node.port() << ">";
  }
}

}  // parser
}  // ssf
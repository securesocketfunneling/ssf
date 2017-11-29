#include <boost/algorithm/string.hpp>

#include <ssf/log/log.h>

#include "common/config/services.h"

namespace ssf {
namespace config {

Services::Services()
    : datagram_forwarder_(),
      datagram_listener_(),
      copy_(),
      shell_(),
      socks_(),
      stream_forwarder_(),
      stream_listener_() {}

Services::Services(const Services& services)
    : datagram_forwarder_(services.datagram_forwarder_),
      datagram_listener_(services.datagram_listener_),
      copy_(services.copy_),
      shell_(services.shell_),
      socks_(services.socks_),
      stream_forwarder_(services.stream_forwarder_),
      stream_listener_(services.stream_listener_) {}

void Services::Update(const Json& json) {
  UpdateDatagramForwarder(json);
  UpdateDatagramListener(json);
  UpdateStreamForwarder(json);
  UpdateStreamListener(json);

  UpdateShell(json);
  UpdateSocks(json);
  UpdateCopy(json);
}

void Services::SetGatewayPorts(bool gateway_ports) {
  datagram_listener_.set_gateway_ports(gateway_ports);
  stream_listener_.set_gateway_ports(gateway_ports);
}

void Services::Log() const {
  if (datagram_listener_.enabled()) {
    if (datagram_listener_.gateway_ports()) {
      SSF_LOG("config", warn,
              "[microservices][datagram_listener] gateway ports allowed");
    }
  }
  if (stream_listener_.enabled()) {
    if (stream_listener_.gateway_ports()) {
      SSF_LOG("config", warn,
              "[microservices][stream_listener] gateway ports allowed");
    }
  }
  if (shell_.enabled()) {
    SSF_LOG("config", info, "[microservices][shell] path: <{}>",
            process().path());
    std::string args(process().args());
    if (!args.empty()) {
      SSF_LOG("config", info, "[microservices][shell] args: <{}>", args);
    }
  }
}

void Services::LogServiceStatus() const {
  SSF_LOG("status", info, "[microservices][datagram_forwarder]: {}",
          (datagram_forwarder_.enabled() ? "On" : "Off"));
  SSF_LOG("status", info, "[microservices][datagram_listener]: {}",
          (datagram_listener_.enabled() ? "On" : "Off"));
  SSF_LOG("status", info, "[microservices][stream_forwarder]: {}",
          (stream_forwarder_.enabled() ? "On" : "Off"));
  SSF_LOG("status", info, "[microservices]][stream_listener]: {}",
          (stream_listener_.enabled() ? "On" : "Off"));
  SSF_LOG("status", info, "[microservices][copy]: {}",
          (copy_.enabled() ? "On" : "Off"));
  SSF_LOG("status", info, "[microservices][shell]: {}",
          (shell_.enabled() ? "On" : "Off"));
  SSF_LOG("status", info, "[microservices][socks]: {}",
          (socks_.enabled() ? "On" : "Off"));
}

void Services::UpdateDatagramForwarder(const Json& json) {
  if (json.count("datagram_forwarder") == 0) {
    SSF_LOG("config", debug,
            "update datagram_forwarder service: configuration not found");
    return;
  }

  datagram_forwarder_.set_enabled(IsServiceEnabled(
      json.at("datagram_forwarder"), datagram_forwarder_.enabled()));
}

void Services::UpdateDatagramListener(const Json& json) {
  if (json.count("datagram_listener") == 0) {
    SSF_LOG("config", debug,
            "update datagram_listener service: configuration not found");
    return;
  }

  auto& datagram_listener_prop = json.at("datagram_listener");

  datagram_listener_.set_enabled(
      IsServiceEnabled(datagram_listener_prop, datagram_listener_.enabled()));

  if (datagram_listener_prop.count("gateway_ports") == 1) {
    datagram_listener_.set_gateway_ports(
        datagram_listener_prop.at("gateway_ports").get<bool>());
  }
}

void Services::UpdateCopy(const Json& json) {
  if (json.count("copy") == 0) {
    SSF_LOG("config", debug, "update copy service: configuration not found");
    return;
  }

  copy_.set_enabled(IsServiceEnabled(json.at("copy"), copy_.enabled()));
}

void Services::UpdateShell(const Json& json) {
  if (json.count("shell") == 0) {
    SSF_LOG("config", debug, "update shell service: configuration not found");
    return;
  }

  auto& shell_prop = json.at("shell");
  shell_.set_enabled(IsServiceEnabled(shell_prop, shell_.enabled()));

  if (shell_prop.count("path") == 1) {
    std::string shell_path(shell_prop.at("path").get<std::string>());
    boost::trim(shell_path);
    shell_.set_path(shell_path);
  }

  if (shell_prop.count("args") == 1) {
    std::string shell_args(shell_prop.at("args").get<std::string>());
    boost::trim(shell_args);
    shell_.set_args(shell_args);
  }
}

void Services::UpdateSocks(const Json& json) {
  if (json.count("socks") == 0) {
    SSF_LOG("config", debug, "update socks service: configuration not found");
    return;
  }

  socks_.set_enabled(IsServiceEnabled(json.at("socks"), socks_.enabled()));
}

void Services::UpdateStreamForwarder(const Json& json) {
  if (json.count("stream_forwarder") == 0) {
    SSF_LOG("config", debug,
            "update stream_forwarder service: configuration not found");
    return;
  }

  stream_forwarder_.set_enabled(IsServiceEnabled(json.at("stream_forwarder"),
                                                 stream_forwarder_.enabled()));
}

void Services::UpdateStreamListener(const Json& json) {
  if (json.count("stream_listener") == 0) {
    SSF_LOG("config", debug,
            "update stream_listener service: configuration not found");
    return;
  }

  auto& stream_listener_prop = json.at("stream_listener");

  stream_listener_.set_enabled(
      IsServiceEnabled(stream_listener_prop, stream_listener_.enabled()));

  if (stream_listener_prop.count("gateway_ports") == 1) {
    stream_listener_.set_gateway_ports(
        stream_listener_prop.at("gateway_ports").get<bool>());
  }
}

bool Services::IsServiceEnabled(const Json& service_json, bool default_value) {
  if (service_json.count("enable") == 1) {
    return service_json.at("enable").get<bool>();
  } else {
    return default_value;
  }
}

}  // config
}  // ssf
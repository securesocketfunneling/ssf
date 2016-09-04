#include <ssf/log/log.h>

#include "common/config/services.h"

namespace ssf {
namespace config {

Services::Services()
    : datagram_forwarder_(),
      datagram_listener_(),
      file_copy_(),
      shell_(),
      socks_(),
      stream_forwarder_(),
      stream_listener_() {}

Services::Services(const Services& services)
    : datagram_forwarder_(services.datagram_forwarder_),
      datagram_listener_(services.datagram_listener_),
      file_copy_(services.file_copy_),
      shell_(services.shell_),
      socks_(services.socks_),
      stream_forwarder_(services.stream_forwarder_),
      stream_listener_(services.stream_listener_) {}

void Services::Update(const PTree& pt) {
  UpdateDatagramForwarder(pt);
  UpdateDatagramListener(pt);
  UpdateStreamForwarder(pt);
  UpdateStreamListener(pt);

  UpdateShell(pt);
  UpdateSocks(pt);
  UpdateFileCopy(pt);
}

void Services::Log() const {
  if (shell_.enabled()) {
    SSF_LOG(kLogInfo) << "config[microservices][shell]: path: <"
                      << process().path() << ">";
    std::string args(process().args());
    if (!args.empty()) {
      SSF_LOG(kLogInfo) << "config[microservices][shell]: args: <" << args
                        << ">";
    }
  }
}

void Services::LogServiceStatus() const {
  SSF_LOG(kLogInfo) << "status[microservices] datagram_forwarder: "
                    << (datagram_forwarder_.enabled() ? "On" : "Off");
  SSF_LOG(kLogInfo) << "status[microservices] datagram_listener: "
                    << (datagram_listener_.enabled() ? "On" : "Off");
  SSF_LOG(kLogInfo) << "status[microservices] stream_forwarder: "
                    << (stream_forwarder_.enabled() ? "On" : "Off");
  SSF_LOG(kLogInfo) << "status[microservices] stream_listener: "
                    << (stream_listener_.enabled() ? "On" : "Off");
  SSF_LOG(kLogInfo) << "status[microservices] file_copy: "
                    << (file_copy_.enabled() ? "On" : "Off");
  SSF_LOG(kLogInfo) << "status[microservices] shell: "
                    << (shell_.enabled() ? "On" : "Off");
  SSF_LOG(kLogInfo) << "status[microservices] socks: "
                    << (socks_.enabled() ? "On" : "Off");
}

void Services::UpdateDatagramForwarder(const PTree& pt) {
  auto optional = pt.get_child_optional("datagram_forwarder");
  if (!optional) {
    SSF_LOG(kLogDebug)
        << "config[update]: datagram_forwarder service configuration not found";
    return;
  }

  datagram_forwarder_.set_enabled(
      ServiceEnabled(optional.get(), datagram_forwarder_.enabled()));
}

void Services::UpdateDatagramListener(const PTree& pt) {
  auto optional = pt.get_child_optional("datagram_listener");
  if (!optional) {
    SSF_LOG(kLogDebug)
        << "config[update]: datagram_listener service configuration not found";
    return;
  }

  datagram_listener_.set_enabled(
      ServiceEnabled(optional.get(), datagram_listener_.enabled()));
}

void Services::UpdateFileCopy(const PTree& pt) {
  auto file_copy_optional = pt.get_child_optional("file_copy");
  if (!file_copy_optional) {
    SSF_LOG(kLogDebug)
        << "config[update]: file_copy service configuration not found";
    return;
  }

  file_copy_.set_enabled(
      ServiceEnabled(file_copy_optional.get(), file_copy_.enabled()));
}

void Services::UpdateShell(const PTree& pt) {
  auto shell_optional = pt.get_child_optional("shell");
  if (!shell_optional) {
    SSF_LOG(kLogDebug)
        << "config[update]: shell service configuration not found";
    return;
  }

  auto& shell_prop = shell_optional.get();
  shell_.set_enabled(ServiceEnabled(shell_prop, shell_.enabled()));

  auto path = shell_prop.get_child_optional("path");
  if (path) {
    shell_.set_path(path.get().data());
  }
  auto args = shell_prop.get_child_optional("args");
  if (args) {
    shell_.set_args(args.get().data());
  }
}

void Services::UpdateSocks(const PTree& pt) {
  auto socks_optional = pt.get_child_optional("socks");
  if (!socks_optional) {
    SSF_LOG(kLogDebug)
        << "config[update]: socks service configuration not found";
    return;
  }

  socks_.set_enabled(ServiceEnabled(socks_optional.get(), socks_.enabled()));
}

void Services::UpdateStreamForwarder(const PTree& pt) {
  auto optional = pt.get_child_optional("stream_forwarder");
  if (!optional) {
    SSF_LOG(kLogDebug)
        << "config[update]: stream_forwarder service configuration not found";
    return;
  }

  stream_forwarder_.set_enabled(
      ServiceEnabled(optional.get(), stream_forwarder_.enabled()));
}

void Services::UpdateStreamListener(const PTree& pt) {
  auto optional = pt.get_child_optional("stream_listener");
  if (!optional) {
    SSF_LOG(kLogDebug)
        << "config[update]: stream_listener service configuration not found";
    return;
  }

  stream_listener_.set_enabled(
      ServiceEnabled(optional.get(), stream_listener_.enabled()));
}

bool Services::ServiceEnabled(const PTree& service_ptree, bool default_value) {
  auto enable_prop = service_ptree.get_child_optional("enable");
  if (enable_prop) {
    return enable_prop.get().get_value<bool>();
  } else {
    return default_value;
  }
}

}  // config
}  // ssf
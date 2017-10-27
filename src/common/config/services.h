#ifndef SSF_COMMON_CONFIG_SERVICES_H_
#define SSF_COMMON_CONFIG_SERVICES_H_

#include <boost/property_tree/ptree.hpp>
#include <boost/system/error_code.hpp>

#include "services/copy/config.h"
#include "services/datagrams_to_fibers/config.h"
#include "services/fibers_to_sockets/config.h"
#include "services/fibers_to_datagrams/config.h"
#include "services/process/config.h"
#include "services/sockets_to_fibers/config.h"
#include "services/socks/config.h"

namespace ssf {
namespace config {

class Services {
 public:
  using PTree = boost::property_tree::ptree;

  using DatagramForwarderConfig = ssf::services::fibers_to_datagrams::Config;
  using DatagramListenerConfig = ssf::services::datagrams_to_fibers::Config;
  using CopyConfig = ssf::services::copy::Config;
  using ShellConfig = ssf::services::process::Config;
  using SocksConfig = ssf::services::socks::Config;
  using StreamForwarderConfig = ssf::services::fibers_to_sockets::Config;
  using StreamListenerConfig = ssf::services::sockets_to_fibers::Config;

 public:
  Services();
  Services(const Services& services);

  const DatagramForwarderConfig& datagram_forwarder() const {
    return datagram_forwarder_;
  }

  DatagramForwarderConfig* mutable_datagram_forwarder() {
    return &datagram_forwarder_;
  }

  const DatagramListenerConfig& datagram_listener() const {
    return datagram_listener_;
  }

  DatagramListenerConfig* mutable_datagram_listener() {
    return &datagram_listener_;
  }

  const ShellConfig& process() const { return shell_; }

  ShellConfig* mutable_process() { return &shell_; }

  const SocksConfig& socks() const { return socks_; }

  SocksConfig* mutable_socks() { return &socks_; }

  const CopyConfig& copy() const { return copy_; }

  CopyConfig* mutable_copy() { return &copy_; }

  const StreamForwarderConfig& stream_forwarder() const {
    return stream_forwarder_;
  }

  StreamForwarderConfig* mutable_stream_forwarder() {
    return &stream_forwarder_;
  }

  const StreamListenerConfig& stream_listener() const {
    return stream_listener_;
  }

  StreamListenerConfig* mutable_stream_listener() { return &stream_listener_; }

  void Update(const PTree& pt);

  // Set gateway ports on listener microservices
  void SetGatewayPorts(bool gateway_ports);

  void Log() const;

  void LogServiceStatus() const;

 private:
  void UpdateDatagramForwarder(const PTree& pt);
  void UpdateDatagramListener(const PTree& pt);
  void UpdateCopy(const PTree& pt);
  void UpdateShell(const PTree& pt);
  void UpdateSocks(const PTree& pt);
  void UpdateStreamForwarder(const PTree& pt);
  void UpdateStreamListener(const PTree& pt);

  static bool ServiceEnabled(const PTree& service, bool default_value);

 private:
  DatagramForwarderConfig datagram_forwarder_;
  DatagramListenerConfig datagram_listener_;
  CopyConfig copy_;
  ShellConfig shell_;
  SocksConfig socks_;
  StreamForwarderConfig stream_forwarder_;
  StreamListenerConfig stream_listener_;
};

}  // config
}  // ssf

#endif  // SSF_COMMON_CONFIG_SERVICES_H_
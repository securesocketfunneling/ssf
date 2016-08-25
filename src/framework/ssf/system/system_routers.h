#ifndef SSF_CORE_SYSTEM_SYSTEM_ROUTERS_H_
#define SSF_CORE_SYSTEM_SYSTEM_ROUTERS_H_

#include <cstdint>

#include <map>
#include <memory>
#include <string>

#include <boost/asio/io_service.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/system/error_code.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include "ssf/layer/network/basic_network_protocol.h"
#include "ssf/layer/routing/basic_routed_protocol.h"

#include "ssf/system/system_interfaces.h"

namespace ssf {
namespace system {

class SystemRouters {
 private:
  using InterfaceProtocol =
      ssf::layer::interface_layer::basic_InterfaceProtocol;
  using NetworkProtocol =
      ssf::layer::network::basic_NetworkProtocol<InterfaceProtocol>;

 public:
  using RoutedProtocol =
      ssf::layer::routing::basic_RoutedProtocol<NetworkProtocol>;
  using Router = RoutedProtocol::router_type;

 private:
  using interface_id = InterfaceProtocol::endpoint_context_type;
  using network_address_type = RoutedProtocol::network_address_type;
  using router_prefix_type = RoutedProtocol::prefix_type;
  using PropertyTree = boost::property_tree::ptree;
  using AllRoutersUpHandler =
      std::function<void(const boost::system::error_code&)>;

 private:
  struct MountNetworkInfo {
    std::string router_name;
    network_address_type network_id;
    NetworkProtocol::resolver::query network_query;
  };

 public:
  explicit SystemRouters(boost::asio::io_service& io_service);

  SystemRouters(const SystemRouters&) = delete;

  SystemRouters& operator=(const SystemRouters&) = delete;

  uint32_t AsyncConfig(const std::string& interfaces_config_filepath,
                       const std::string& routers_config_filepath,
                       boost::system::error_code& ec,
                       AllRoutersUpHandler all_up_handler =
                           [](const boost::system::error_code&) {});

  void Start(int remount_delay = SystemInterfaces::DEFAULT_REMOUNT_DELAY_SEC);

  void Stop();

  void StopAllRouters();

  void StopRouter(const std::string& router_name);

 private:
  void AddNetworksFromPropertyTree(const std::string& router_name,
                                   const PropertyTree& networks_ptree,
                                   boost::system::error_code& ec);

  void AddRoutesFromPropertyTree(Router* p_router,
                                 const PropertyTree& routes_ptree,
                                 boost::system::error_code& ec);

  void InterfaceUpHandler(const boost::system::error_code& ec,
                          const interface_id& interface_name,
                          AllRoutersUpHandler all_up_handler =
                              [](const boost::system::error_code&) {});

  void PostAllUpHandler(const boost::system::error_code& ec,
                        AllRoutersUpHandler all_up_handler);

 private:
  boost::asio::io_service& io_service_;
  ssf::system::SystemInterfaces system_interfaces_;
  std::map<std::string, std::shared_ptr<Router>> p_routers_;
  boost::recursive_mutex mount_infos_mutex_;
  std::map<interface_id, MountNetworkInfo> mount_infos_;
};

}  // system
}  // ssf

#endif  // SSF_CORE_SYSTEM_SYSTEM_ROUTERS_H_

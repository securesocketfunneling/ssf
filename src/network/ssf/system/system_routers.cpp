#include "ssf/system/system_routers.h"

#include <cstdint>

#include <map>
#include <string>
#include <unordered_set>

#include <boost/asio/io_service.hpp>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/system/error_code.hpp>

#include "ssf/error/error.h"
#include "ssf/layer/parameters.h"

#include "ssf/layer/data_link/basic_circuit_protocol.h"
#include "ssf/layer/data_link/simple_circuit_policy.h"
#include "ssf/layer/data_link/circuit_helpers.h"
#include "ssf/layer/interface_layer/basic_interface_protocol.h"
#include "ssf/layer/physical/tcp.h"
#include "ssf/layer/physical/tlsotcp.h"

#include "ssf/log/log.h"

namespace ssf {
namespace system {

SystemRouters::SystemRouters(boost::asio::io_service& io_service)
    : io_service_(io_service),
      system_interfaces_(io_service),
      p_routers_(),
      mount_infos_mutex_(),
      mount_infos_() {}

uint32_t SystemRouters::AsyncConfig(
    const std::string& interfaces_config_filepath,
    const std::string& routers_config_filepath, boost::system::error_code& ec,
    SystemRouters::AllRoutersUpHandler all_up_handler) {
  PropertyTree pt;

  try {
    boost::property_tree::read_json(routers_config_filepath, pt);
  } catch (const std::exception&) {
    ec.assign(ssf::error::bad_file_descriptor, ssf::error::get_ssf_category());
    io_service_.post(boost::asio::detail::binder1<AllRoutersUpHandler,
                                                  boost::system::error_code>(
        all_up_handler, ec));
    return 0;
  }

  std::unordered_set<std::string> added_routers;

  for (auto& config_router : pt) {
    auto given_router_name = config_router.second.get_child_optional("router");
    if (!given_router_name) {
      ec.assign(ssf::error::missing_config_parameters,
                ssf::error::get_ssf_category());
      break;
    }

    std::string router_name = given_router_name->data().c_str();
    auto p_router = RoutedProtocol::get_router(router_name);

    // A router already exists with this name, stop
    if (p_router) {
      ec.assign(ssf::error::device_or_resource_busy,
                ssf::error::get_ssf_category());
      break;
    }

    RoutedProtocol::StartRouter(router_name, io_service_);
    added_routers.insert(router_name);
    p_router = RoutedProtocol::get_router(router_name);
    p_routers_[router_name] = p_router;

    const auto& given_networks = config_router.second.get_child_optional("networks");
    const auto& given_routes = config_router.second.get_child_optional("routes");

    if (!given_networks && !given_routes) {
      ec.assign(ssf::error::missing_config_parameters,
                ssf::error::get_ssf_category());
      break;
    }

    if (given_networks) {
      AddNetworksFromPropertyTree(router_name, *given_networks, ec);
    }

    if (!ec) {
      if (given_routes) {
        AddRoutesFromPropertyTree(p_router.get(), *given_routes, ec);
      }
    }

    if (ec) {
      break;
    }
  }

  // If an error occured, stop all previous routers configured
  if (ec) {
    SSF_LOG(kLogError) << "Error when configuring router";
    for (auto& router_name : added_routers) {
      SSF_LOG(kLogError) << " * Remove router " << router_name;
      StopRouter(router_name);
    }
    added_routers.clear();
    io_service_.post(boost::asio::detail::binder1<AllRoutersUpHandler,
                                                  boost::system::error_code>(
        all_up_handler, ec));
  }

  system_interfaces_.AsyncMount(interfaces_config_filepath,
                                boost::bind(&SystemRouters::InterfaceUpHandler,
                                            this, _1, _2, all_up_handler));

  return static_cast<uint32_t>(added_routers.size());
}

void SystemRouters::Start(int remount_delay) {
  using TCPProtocol = ssf::layer::physical::TCPPhysicalLayer;
  using TLSoTCPProtocol =
      ssf::layer::physical::TLSboTCPPhysicalLayer;
  using CircuitTCPProtocol =
      ssf::layer::data_link::basic_CircuitProtocol<
          TCPProtocol, ssf::layer::data_link::CircuitPolicy>;
  using CircuitTLSoTCPProtocol =
      ssf::layer::data_link::basic_CircuitProtocol<
          TLSoTCPProtocol, ssf::layer::data_link::CircuitPolicy>;

  system_interfaces_.RegisterInterfacesCollection<TCPProtocol>();
  system_interfaces_.RegisterInterfacesCollection<TLSoTCPProtocol>();
  system_interfaces_.RegisterInterfacesCollection<CircuitTCPProtocol>();
  system_interfaces_.RegisterInterfacesCollection<CircuitTLSoTCPProtocol>();

  system_interfaces_.Start(remount_delay);
}

void SystemRouters::Stop() {
  StopAllRouters();
  system_interfaces_.Stop();
  system_interfaces_.UnregisterAllInterfacesCollection();
}

void SystemRouters::StopAllRouters() {
  auto p_router_it = p_routers_.begin();
  auto end_router_it = p_routers_.end();
  while (p_router_it != end_router_it) {
    boost::system::error_code ec;
    RoutedProtocol::StopRouter(p_router_it->first, ec);
    p_router_it = p_routers_.erase(p_router_it);
  }
}

void SystemRouters::StopRouter(const std::string& router_name) {
  auto p_router_it = p_routers_.find(router_name);
  if (p_router_it != p_routers_.end()) {
    boost::system::error_code ec;
    RoutedProtocol::StopRouter(router_name, ec);
    p_routers_.erase(router_name);
  }
}

void SystemRouters::AddNetworksFromPropertyTree(const std::string& router_name,
                                                const PropertyTree& networks_pt,
                                                boost::system::error_code& ec) {
  for (auto& network_pt : networks_pt) {
    network_address_type network_id =
        RoutedProtocol::ResolveToNetworkAddress(network_pt.first.c_str(), ec);
    if (ec) {
      return;
    }
    std::string interface_id = network_pt.second.data().c_str();
    ssf::layer::LayerParameters network_parameters;
    network_parameters["network_id"] = network_pt.first.c_str();
    ssf::layer::LayerParameters interface_parameters;
    interface_parameters["interface_id"] = interface_id;

    ssf::layer::ParameterStack network_query;
    network_query.push_back(network_parameters);
    network_query.push_back(interface_parameters);

    MountNetworkInfo mount_info;
    mount_info.network_id = network_id;
    mount_info.network_query = network_query;
    mount_info.router_name = router_name;

    {
      boost::recursive_mutex::scoped_lock lock_mount_infos(mount_infos_mutex_);
      mount_infos_.emplace(interface_id, mount_info);
    }
  }
}

void SystemRouters::AddRoutesFromPropertyTree(SystemRouters::Router* p_router,
                                              const PropertyTree& routes_pt,
                                              boost::system::error_code& ec) {
  for (auto& route_pt : routes_pt) {
    network_address_type from_network_id =
        RoutedProtocol::ResolveToNetworkAddress(route_pt.first.c_str(), ec);

    network_address_type to_network_id =
        RoutedProtocol::ResolveToNetworkAddress(route_pt.second.data().c_str(),
                                                ec);
    if (ec) {
      return;
    }
    p_router->add_route(from_network_id, to_network_id, ec);
    if (ec) {
      return;
    }
  }
}

void SystemRouters::InterfaceUpHandler(
    const boost::system::error_code& ec,
    const SystemRouters::interface_id& interface_name,
    SystemRouters::AllRoutersUpHandler all_up_handler) {
  boost::recursive_mutex::scoped_lock lock_mount_infos(mount_infos_mutex_);
  auto mount_info_it = mount_infos_.find(interface_name);
  if (mount_info_it == mount_infos_.end()) {
    return;
  }

  auto& mount_info = mount_info_it->second;

  if (ec) {
    SSF_LOG(kLogError) << " * Interface " << interface_name
                       << " error : " << ec.message();
    mount_infos_.erase(interface_name);
    PostAllUpHandler(ec, all_up_handler);
    return;
  }

  auto p_router = RoutedProtocol::get_router(mount_info_it->second.router_name);
  if (!p_router) {
    mount_infos_.erase(mount_info_it);
    PostAllUpHandler(boost::system::error_code(ssf::error::identifier_removed,
                                               ssf::error::get_ssf_category()),
                     all_up_handler);
    return;
  }

  NetworkProtocol::resolver network_resolver(io_service_);
  boost::system::error_code add_network_ec;
  auto network_ep_it =
      network_resolver.resolve(mount_info.network_query, add_network_ec);
  if (ec) {
    mount_infos_.erase(mount_info_it);
    PostAllUpHandler(
        boost::system::error_code(ssf::error::cannot_resolve_endpoint,
                                  ssf::error::get_ssf_category()),
        all_up_handler);
    return;
  }

  p_router->add_network(mount_info.network_id, *network_ep_it, add_network_ec);
  mount_infos_.erase(mount_info_it);

  PostAllUpHandler(boost::system::error_code(ssf::error::success,
                                             ssf::error::get_ssf_category()),
                   all_up_handler);
}

void SystemRouters::PostAllUpHandler(const boost::system::error_code& all_up_ec,
                                     AllRoutersUpHandler all_up_handler) {
  boost::recursive_mutex::scoped_lock lock_mount_infos(mount_infos_mutex_);
  if (mount_infos_.empty()) {
    io_service_.post(boost::asio::detail::binder1<AllRoutersUpHandler,
                                                  boost::system::error_code>(
        all_up_handler, all_up_ec));
  }
}

}  // system
}  // ssf

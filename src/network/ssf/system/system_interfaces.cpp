#include <cstdint>

#include <functional>
#include <memory>
#include <mutex>
#include <string>

#include <boost/asio/io_service.hpp>

#include <boost/optional.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/system/error_code.hpp>

#include "ssf/error/error.h"

#include "ssf/system/specific_interfaces_collection.h"
#include "ssf/system/system_interfaces.h"

namespace ssf {
namespace system {

SystemInterfaces::SystemInterfaces(boost::asio::io_service& io_service)
    : io_service_(io_service),
      p_worker_(nullptr),
      interfaces_collections_mutex_(),
      interfaces_collection_map_(),
      remount_timer_(io_service_) {}

SystemInterfaces::~SystemInterfaces() { Stop(); }

void SystemInterfaces::UnregisterAllInterfacesCollection() {
  interfaces_collection_map_.clear();
}

uint32_t SystemInterfaces::AsyncMount(const std::string& config_filepath,
                                      InterfaceUpHandler interface_up_handler) {
  uint32_t nb_interfaces_async_mount = 0;
  SystemInterfaces::PropertyTree pt;

  try {
    boost::property_tree::read_json(config_filepath, pt);
  } catch (const std::exception&) {
    return 0;
  }

  for (auto& config_interface : pt) {
    auto layer_stack =
        config_interface.second.get_child_optional("layer_stack");
    if (!layer_stack) {
      continue;
    }

    auto optional_name = GetCollectionNameFromLayerStack(*layer_stack);

    if (!optional_name) {
      continue;
    }

    {
      std::unique_lock<std::recursive_mutex> lock_interfaces_collections(
          interfaces_collections_mutex_);
      auto interfaces_collection_it =
          interfaces_collection_map_.find(*optional_name);
      if (interfaces_collection_it == interfaces_collection_map_.end()) {
        continue;
      }

      interfaces_collection_it->second->AsyncMount(
          io_service_, config_interface.second, interface_up_handler);
      ++nb_interfaces_async_mount;
    }
  }

  return nb_interfaces_async_mount;
}

void SystemInterfaces::UmountAll() {
  std::unique_lock<std::recursive_mutex> lock_interfaces_collections(
      interfaces_collections_mutex_);
  for (auto& interfaces_collection_pair : interfaces_collection_map_) {
    interfaces_collection_pair.second->UmountAll();
  }
}

void SystemInterfaces::Start(int remount_delay) {
  if (!p_worker_) {
    p_worker_ = std::unique_ptr<boost::asio::io_service::work>(
        new boost::asio::io_service::work(io_service_));
    LaunchRemountTimer(remount_delay);
  }
}

void SystemInterfaces::Stop() {
  boost::system::error_code ec;
  UmountAll();
  remount_timer_.cancel(ec);
  if (p_worker_) {
    p_worker_.reset();
  }
}

boost::asio::io_service& SystemInterfaces::get_io_service() {
  return io_service_;
}

boost::optional<std::string> SystemInterfaces::GetCollectionNameFromLayerStack(
    const SystemInterfaces::PropertyTree& property_tree) {
  try {
    std::stringstream ss;
    ss << property_tree.get_child("layer").data();
    auto sublayer = property_tree.get_child_optional("sublayer");
    while (sublayer) {
      ss << "_" << sublayer->get_child("layer").data();
      sublayer = sublayer->get_child_optional("sublayer");
    }

    return ss.str();
  } catch (const std::exception&) {
    return boost::optional<std::string>();
  }
}

void SystemInterfaces::LaunchRemountTimer(int remount_delay) {
  remount_timer_.expires_from_now(std::chrono::seconds(remount_delay));
  remount_timer_.async_wait(std::bind(&SystemInterfaces::RemountTimerHandler,
                                      this, std::placeholders::_1,
                                      remount_delay));
}

void SystemInterfaces::RemountTimerHandler(const boost::system::error_code& ec,
                                           int remount_delay) {
  if (!ec) {
    for (auto& interfaces_collection_pair : interfaces_collection_map_) {
      interfaces_collection_pair.second->RemountDownInterfaces();
    }
    LaunchRemountTimer(remount_delay);
  }
}

}  // system
}  // ssf

#ifndef SSF_CORE_SYSTEM_SYSTEM_INTERFACES_H_
#define SSF_CORE_SYSTEM_SYSTEM_INTERFACES_H_

#include <cstdint>

#include <map>
#include <memory>
#include <string>

#include <boost/asio/io_service.hpp>
#include <boost/asio/steady_timer.hpp>

#include <boost/optional.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/system/error_code.hpp>
#include <boost/thread.hpp>

#include "core/system/basic_interfaces_collection.h"
#include "core/system/specific_interfaces_collection.h"

namespace ssf {
namespace system {

class SystemInterfaces {
 public:
  using PropertyTree = boost::property_tree::ptree;
  using InterfacesCollectionPtr = std::unique_ptr<BasicInterfacesCollection>;
  using InterfaceUpHandler = BasicInterfacesCollection::MountCallback;

  enum { DEFAULT_REMOUNT_DELAY_SEC = 60 };

 public:
  SystemInterfaces();

  SystemInterfaces(const SystemInterfaces&) = delete;

  SystemInterfaces& operator=(const SystemInterfaces&) = delete;

  template <class LayerStack>
  bool RegisterInterfacesCollection() {
    std::string stack_id(LayerStack::get_name());

    if (interfaces_collection_map_.find(stack_id) !=
        interfaces_collection_map_.end()) {
      return false;
    }

    interfaces_collection_map_.emplace(
        stack_id, std::unique_ptr<SpecificInterfacesCollection<LayerStack>>(
          new SpecificInterfacesCollection<LayerStack>()));

    return true;
  }

  template <class LayerStack>
  void UnregisterInterfacesCollection() {
    interfaces_collection_map_.erase(LayerStack::get_name());
  }

  void UnregisterAllInterfacesCollection();

  uint32_t AsyncMount(const std::string& config_filepath,
                      InterfaceUpHandler interface_up_handler);

  void UmountAll();

  void Start(uint16_t nb_threads = boost::thread::hardware_concurrency(),
             int remount_delay = DEFAULT_REMOUNT_DELAY_SEC);

  void Stop();

 private:
  boost::optional<std::string> GetCollectionNameFromLayerStack(
      const PropertyTree& property_tree);

  void LaunchRemountTimer(int remount_delay);

  void RemountTimerHandler(const boost::system::error_code& ec,
                           int remount_delay);

 private:
  boost::asio::io_service io_service_;
  std::unique_ptr<boost::asio::io_service::work> p_worker_;
  std::unique_ptr<boost::thread_group> p_threads_;
  boost::recursive_mutex interfaces_collections_mutex_;
  std::map<std::string, InterfacesCollectionPtr> interfaces_collection_map_;
  boost::asio::steady_timer remount_timer_;
};

}  // system
}  // ssf

#endif  // SSF_CORE_SYSTEM_SYSTEM_INTERFACES_H_

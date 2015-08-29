#ifndef SSF_CORE_SYSTEM_SPECIFIC_INTERFACES_COLLECTION_H_
#define SSF_CORE_SYSTEM_SPECIFIC_INTERFACES_COLLECTION_H_

#include <chrono>
#include <string>
#include <unordered_set>

#include <boost/asio/steady_timer.hpp>

#include <boost/log/trivial.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/system/error_code.hpp>

#include "common/error/error.h"
#include "core/virtual_network/interface_layer/basic_interface_protocol.h"
#include "core/virtual_network/interface_layer/basic_interface.h"

#include "core/system/basic_interfaces_collection.h"

namespace ssf {
namespace system {

template <class LayerStack>
class SpecificInterfacesCollection : public BasicInterfacesCollection {
 public:
  using PropertyTree = boost::property_tree::ptree;

  using InterfaceProtocol =
      virtual_network::interface_layer::basic_InterfaceProtocol;

  template <class NextLayer>
  using Interface =
      virtual_network::interface_layer::basic_Interface<InterfaceProtocol,
                                                        LayerStack>;

  using MountCallback = BasicInterfacesCollection::MountCallback;

  typedef typename LayerStack::endpoint Endpoint;
  typedef typename LayerStack::resolver Resolver;

 private:
  using TimerPtr = std::shared_ptr<boost::asio::steady_timer>;

  struct InterfaceConfig {
    bool connect;
    Endpoint endpoint;
    int ttl;
    int delay;
    MountCallback mount_callback;
  };

 public:
  enum { DEFAULT_TTL = 10, DEFAULT_DELAY = 1000 };

 public:
  explicit SpecificInterfacesCollection() : interfaces_() {}

  virtual ~SpecificInterfacesCollection() {
    UmountAll();
  }

  virtual std::string GetName() { return LayerStack::get_name(); }

  virtual void AsyncMount(boost::asio::io_service& io_service,
                          const PropertyTree& property_tree,
                          MountCallback mount_handler) {
    boost::system::error_code ec;
    InterfaceConfig config;
    typename Resolver::query query;

    config.connect = true;
    config.ttl = DEFAULT_TTL;
    config.delay = DEFAULT_DELAY;
    config.mount_callback = mount_handler;

    auto given_interface_name = property_tree.get_child_optional("interface");
    if (!given_interface_name) {
      io_service.post(
          boost::asio::detail::binder2<MountCallback, boost::system::error_code,
                                       std::string>(
              mount_handler,
              boost::system::error_code(ssf::error::missing_config_parameters,
                                        ssf::error::get_ssf_category()),
              ""));
      return;
    }
    std::string interface_name = given_interface_name.get().data();

    auto given_type = property_tree.get_child_optional("type");
    if (given_type && given_type.get().data() == "ACCEPT") {
      config.connect = false;
    }
    auto given_ttl = property_tree.get_child_optional("ttl");
    if (given_ttl) {
      config.ttl = given_ttl->get_value<int>();
    }
    auto given_delay = property_tree.get_child_optional("delay");
    if (given_delay) {
      config.delay = given_delay->get_value<int>();
    }

    PropertyTreeToQuery(&query, property_tree, config.connect, ec);

    Resolver resolver(io_service);
    auto endpoint_it = resolver.resolve(query, ec);
    if (ec) {
      io_service.post(
          boost::asio::detail::binder2<MountCallback, boost::system::error_code,
                                       std::string>(
              mount_handler,
              boost::system::error_code(ssf::error::cannot_resolve_endpoint,
                                        ssf::error::get_ssf_category()),
              interface_name));
      return;
    }

    config.endpoint = *endpoint_it;

    {
      boost::recursive_mutex::scoped_lock lock_interfaces(interfaces_mutex_);
      if (interfaces_.find(interface_name) != interfaces_.end()) {
        io_service.post(boost::asio::detail::binder2<
            MountCallback, boost::system::error_code, std::string>(
            mount_handler,
            boost::system::error_code(ssf::error::address_not_available,
                                      ssf::error::get_ssf_category()),
            interface_name));
        return;
      }
      interfaces_.emplace(interface_name, io_service);
      interfaces_config_.emplace(interface_name, config);

      InitializeInterface(interface_name, config.ttl);
    }
  }

  virtual void RemountDownInterfaces() {
    boost::recursive_mutex::scoped_lock lock_interfaces(interfaces_mutex_);
    auto interface_up_it = interfaces_up_.begin();
    auto interface_up_end_it = interfaces_up_.end();

    while (interface_up_it != interface_up_end_it) {
      auto interface_it = interfaces_.find(*interface_up_it);
      if (interface_it == interfaces_.end()) {
        interface_up_it = interfaces_up_.erase(interface_up_it);
        continue;
      } else {
        auto& config = interfaces_config_[*interface_up_it];
        InitializeInterface(*interface_up_it, config.ttl);
        ++interface_up_it;
      }
    }
  }

  virtual void Umount(const std::string& interface_name) {
    boost::recursive_mutex::scoped_lock lock_interfaces(interfaces_mutex_);
    auto interface_it = interfaces_.find(interface_name);
    if (interface_it != interfaces_.end()) {
      interfaces_.erase(interface_it);
      interfaces_up_.erase(interface_name);
      interfaces_config_.erase(interface_name);
    }
  }

  virtual void UmountAll() {
    boost::recursive_mutex::scoped_lock lock_interfaces(interfaces_mutex_);
    auto interface_it = interfaces_.begin();
    auto interface_end_it = interfaces_.end();
    while (interface_it != interface_end_it) {
      interfaces_config_.erase(interface_it->first);
      interfaces_up_.erase(interface_it->first);
      interface_it = interfaces_.erase(interface_it);
    }
  }

 private:
  void PropertyTreeToQuery(typename Resolver::query* p_query,
                           const PropertyTree& property_tree, bool connect,
                           boost::system::error_code& ec) {
    auto layer_stack = property_tree.get_child_optional("layer_stack");
    if (!layer_stack) {
      ec.assign(ssf::error::missing_config_parameters,
                ssf::error::get_ssf_category());
      return;
    }

    LayerStack::add_params_from_property_tree(p_query, *layer_stack, connect,
                                              ec);
  }

  void InitializeInterface(const std::string& interface_name,
                           int ttl = DEFAULT_TTL, TimerPtr p_timer = nullptr) {
    boost::recursive_mutex::scoped_lock lock_interfaces(interfaces_mutex_);
    auto interface_it = interfaces_.find(interface_name);
    if (interface_it == interfaces_.end()) {
      return;
    }
    auto& config = interfaces_config_[interface_name];

    if (p_timer == nullptr) {
      p_timer = std::make_shared<boost::asio::steady_timer>(
          interface_it->second.get_io_service());
    }

    if (config.connect) {
      interface_it->second.async_connect(
          interface_name, config.endpoint,
          boost::bind(&SpecificInterfacesCollection::MountConnectHandler,
                      this, _1, interface_name, p_timer, ttl - 1));
    } else {
      int timeout = config.ttl * config.delay;
      if (timeout > 0) {
        p_timer->expires_from_now(std::chrono::milliseconds(timeout));
        p_timer->async_wait(
            boost::bind(&SpecificInterfacesCollection::MountAcceptTimeOut,
                        this, _1, p_timer, interface_name));
      }
      interface_it->second.async_accept(
          interface_name, config.endpoint,
          boost::bind(&SpecificInterfacesCollection::MountAcceptHandler, this,
                      _1, interface_name, p_timer));
    }
  }

  void MountConnectHandler(const boost::system::error_code& ec,
                           std::string interface_name, TimerPtr p_timer,
                           int ttl) {
    boost::recursive_mutex::scoped_lock lock_interfaces(interfaces_mutex_);
    auto interface_it = interfaces_.find(interface_name);
    if (interface_it == interfaces_.end()) {
      return;
    }

    auto& config = interfaces_config_[interface_name];
    auto handler = config.mount_callback;

    if (ec) {
      if (ttl <= 0) {
        interfaces_.erase(interface_name);
        interfaces_config_.erase(interface_name);
        handler(boost::system::error_code(ssf::error::connection_aborted,
                                          ssf::error::get_ssf_category()),
                interface_name);
        return;
      }

      p_timer->expires_from_now(std::chrono::milliseconds(config.delay));
      p_timer->async_wait([this, interface_name, p_timer, ttl](
          const boost::system::error_code& ec) {
        InitializeInterface(interface_name, ttl, p_timer);
      });

      return;
    }

    interfaces_up_.insert(interface_name);

    handler(boost::system::error_code(ssf::error::success,
                                      ssf::error::get_ssf_category()),
            interface_name);
  }

  void MountAcceptHandler(const boost::system::error_code& ec,
                          std::string interface_name, TimerPtr p_timer) {
    boost::recursive_mutex::scoped_lock lock_interfaces(interfaces_mutex_);

    p_timer->cancel();

    auto interface_it = interfaces_.find(interface_name);
    if (interface_it == interfaces_.end()) {
      return;
    }

    auto& config = interfaces_config_[interface_name];
    auto handler = config.mount_callback;

    if (ec) {
      interfaces_.erase(interface_name);
      interfaces_config_.erase(interface_name);
      interfaces_up_.erase(interface_name);
      handler(boost::system::error_code(ssf::error::connection_aborted,
                                        ssf::error::get_ssf_category()),
              interface_name);
      return;
    }
    handler(boost::system::error_code(ssf::error::success,
                                      ssf::error::get_ssf_category()),
            interface_name);
  }

  void MountAcceptTimeOut(const boost::system::error_code& ec, TimerPtr p_timer,
                          std::string interface_name) {
    if (!ec) {
      boost::recursive_mutex::scoped_lock lock_interfaces(interfaces_mutex_);
      auto interface_it = interfaces_.find(interface_name);
      if (interface_it == interfaces_.end()) {
        return;
      }
      boost::system::error_code close_ec;
      interface_it->second.close(close_ec);
    }
  }

 private:
  boost::recursive_mutex interfaces_mutex_;
  std::map<std::string, Interface<LayerStack>> interfaces_;
  std::map<std::string, InterfaceConfig> interfaces_config_;
  std::unordered_set<std::string> interfaces_up_;
};

}  // system
}  // ssf

#endif  // SSF_CORE_SYSTEM_SPECIFIC_INTERFACES_COLLECTION_H_

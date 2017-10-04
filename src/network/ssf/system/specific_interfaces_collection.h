#ifndef SSF_SYSTEM_SPECIFIC_INTERFACES_COLLECTION_H_
#define SSF_SYSTEM_SPECIFIC_INTERFACES_COLLECTION_H_

#include <chrono>
#include <string>
#include <unordered_set>

#include <boost/asio/steady_timer.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/system/error_code.hpp>

#include "ssf/error/error.h"
#include "ssf/layer/interface_layer/basic_interface_protocol.h"
#include "ssf/layer/interface_layer/basic_interface.h"

#include "ssf/log/log.h"

#include "ssf/system/basic_interfaces_collection.h"

namespace ssf {
namespace system {

template <class LayerStack>
class SpecificInterfacesCollection : public BasicInterfacesCollection {
 public:
  using PropertyTree = boost::property_tree::ptree;

  using InterfaceProtocol =
      ssf::layer::interface_layer::basic_InterfaceProtocol;

  template <class NextLayer>
  using Interface =
      ssf::layer::interface_layer::basic_Interface<InterfaceProtocol,
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
  enum { DEFAULT_TTL = 1, DEFAULT_DELAY = 0 };

 public:
  explicit SpecificInterfacesCollection() : interfaces_() {}

  virtual ~SpecificInterfacesCollection() { UmountAll(); }

  virtual std::string GetName() { return LayerStack::get_name(); }

  virtual void AsyncMount(boost::asio::io_service& io_service,
                          const PropertyTree& property_tree,
                          MountCallback mount_handler) {
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

    boost::system::error_code ec;
    InterfaceConfig config;
    InitConfig(io_service, &config, property_tree, mount_handler, ec);

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

    while (interface_up_it != interfaces_up_.end()) {
      auto interface_it = interfaces_.find(*interface_up_it);
      // interface no longer managed by this collection
      if (interface_it == interfaces_.end()) {
        interface_up_it = interfaces_up_.erase(interface_up_it);
        continue;
      } else {
        if (!interface_it->second.is_open()) {
          auto& interface_name = *interface_up_it;
          SSF_LOG(kLogTrace) << " * Interface " << interface_name << " down";

          const auto& config = interfaces_config_[interface_name];
          InitializeInterface(*interface_up_it, config.ttl);
        }
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
    while (interface_it != interfaces_.end()) {
      interfaces_config_.erase(interface_it->first);
      interfaces_up_.erase(interface_it->first);
      interface_it = interfaces_.erase(interface_it);
    }
  }

 private:
  void InitConfig(boost::asio::io_service& io_service,
                  InterfaceConfig* p_config, const PropertyTree& property_tree,
                  MountCallback mount_handler, boost::system::error_code& ec) {
    p_config->connect = true;
    p_config->ttl = DEFAULT_TTL;
    p_config->delay = DEFAULT_DELAY;
    p_config->mount_callback = mount_handler;

    auto given_type = property_tree.get_child_optional("type");
    if (given_type && given_type.get().data() == "ACCEPT") {
      p_config->connect = false;
    }
    auto given_ttl = property_tree.get_child_optional("ttl");
    if (given_ttl) {
      p_config->ttl = given_ttl->get_value<int>();
    }
    auto given_delay = property_tree.get_child_optional("delay");
    if (given_delay) {
      p_config->delay = given_delay->get_value<int>();
    }

    typename Resolver::query query;
    InitQuery(&query, property_tree, p_config->connect, ec);

    if (ec) {
      return;
    }

    Resolver resolver(io_service);
    auto endpoint_it = resolver.resolve(query, ec);

    if (ec) {
      return;
    }

    p_config->endpoint = *endpoint_it;
  }

  void InitQuery(typename Resolver::query* p_query,
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
    const auto& config = interfaces_config_[interface_name];

    if (p_timer == nullptr) {
      p_timer = std::make_shared<boost::asio::steady_timer>(
          interface_it->second.get_io_service());
    }

    if (config.connect) {
      interface_it->second.async_connect(
          interface_name, config.endpoint,
          boost::bind(&SpecificInterfacesCollection::MountConnectHandler, this,
                      _1, interface_name, p_timer, ttl - 1));
    } else {
      int timeout = config.ttl * config.delay;
      if (timeout > 0) {
        p_timer->expires_from_now(std::chrono::milliseconds(timeout));
        p_timer->async_wait(
            boost::bind(&SpecificInterfacesCollection::MountAcceptTimeOut, this,
                        _1, p_timer, interface_name));
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
        // the interface was not connected previously, dead end
        if (interfaces_up_.find(interface_name) == interfaces_up_.end()) {
          interface_it->second.get_io_service().post(boost::asio::detail::binder2<
              MountCallback, boost::system::error_code, std::string>(
              handler, boost::system::error_code(ssf::error::connection_aborted,
                                                 ssf::error::get_ssf_category()),
              interface_name));
          interfaces_.erase(interface_name);
          interfaces_config_.erase(interface_name);
        }
        return;
      }

      p_timer->expires_from_now(std::chrono::milliseconds(config.delay));
      p_timer->async_wait([this, interface_name, p_timer, ttl](
          const boost::system::error_code& ec) {
        this->InitializeInterface(interface_name, ttl, p_timer);
      });

      return;
    }

    SSF_LOG(kLogTrace) << " * Interface " << interface_name << " up";
    interfaces_up_.insert(interface_name);

    interface_it->second.get_io_service().post(
        boost::asio::detail::binder2<MountCallback, boost::system::error_code,
                                     std::string>(
            handler, boost::system::error_code(ssf::error::success,
                                               ssf::error::get_ssf_category()),
            interface_name));
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
      // the interface was not accepted previously, dead end
      if (interfaces_up_.find(interface_name) == interfaces_up_.end()) {
        interface_it->second.get_io_service().post(boost::asio::detail::binder2<
            MountCallback, boost::system::error_code, std::string>(
            handler, boost::system::error_code(ssf::error::connection_aborted,
                                               ssf::error::get_ssf_category()),
            interface_name));
        interfaces_.erase(interface_name);
        interfaces_config_.erase(interface_name);
      }
      return;
    }

    interfaces_up_.insert(interface_name);

    interface_it->second.get_io_service().post(
        boost::asio::detail::binder2<MountCallback, boost::system::error_code,
                                     std::string>(
            handler, boost::system::error_code(ssf::error::success,
                                               ssf::error::get_ssf_category()),
            interface_name));
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

#endif  // SSF_SYSTEM_SPECIFIC_INTERFACES_COLLECTION_H_

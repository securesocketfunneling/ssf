#ifndef SSF_CORE_FACTORIES_SERVICE_FACTORY_H_
#define SSF_CORE_FACTORIES_SERVICE_FACTORY_H_

#include <cstdint>

#include <functional>
#include <map>
#include <memory>
#include <mutex>

#include <boost/asio/io_service.hpp>
#include <boost/system/error_code.hpp>

#include "common/error/error.h"

#include "core/service_manager/service_manager.h"
#include "services/base_service.h"

#include "core/factory_manager/service_factory_manager.h"

namespace ssf {

template <typename Demux>
class ServiceFactory
    : public std::enable_shared_from_this<ServiceFactory<Demux>> {
 private:
  using Parameters = std::map<std::string, std::string>;
  using BaseServicePtr = std::shared_ptr<BaseService<Demux>>;
  using ServiceCreator = std::function<BaseServicePtr(boost::asio::io_service&,
                                                      Demux&, const Parameters&)>;
  using ServiceCreatorMap = std::map<uint32_t, ServiceCreator>;
  using ServiceManagerPtr = std::shared_ptr<ServiceManager<Demux>>;

 public:
  static std::shared_ptr<ServiceFactory> Create(
      boost::asio::io_service& io_service, Demux& demux,
      ServiceManagerPtr p_service_manager) {
    auto p_service_factory = std::shared_ptr<ServiceFactory>(
        new ServiceFactory(io_service, demux, p_service_manager));

    if (p_service_factory) {
      ServiceFactoryManager<Demux>::RegisterServiceFactory(&demux,
                                                           p_service_factory);
    }

    return std::move(p_service_factory);
  }

  ~ServiceFactory() {}

  bool RegisterServiceCreator(uint32_t index, ServiceCreator creator) {
    std::unique_lock<std::recursive_mutex> lock(service_creators_mutex_);
    if (service_creators_.count(index)) {
      return false;
    } else {
      service_creators_[index] = creator;
      return true;
    }
  }

  uint32_t CreateRunNewService(uint32_t index, Parameters parameters,
                               boost::system::error_code& ec) {
    std::unique_lock<std::recursive_mutex> lock(service_creators_mutex_);

    auto it = service_creators_.find(index);

    if (it != std::end(service_creators_)) {
      auto& service_creator = it->second;
      auto p_service = service_creator(io_service_, demux_, parameters);
      if (p_service) {
        auto service_id = p_service_manager_->start(p_service, ec);
        p_service->set_local_id(service_id);

        return service_id;
      } else {
        ec.assign(::error::service_not_started, error::get_ssf_category());
        return 0;
      }
    } else {
      ec.assign(::error::service_not_found, error::get_ssf_category());
      return 0;
    }
  }

  void StopService(uint32_t id) {
    boost::system::error_code ec;
    p_service_manager_->stop_with_id(id, ec);
  }

  void StopAllLocalServices() { p_service_manager_->stop_all(); }

  void Destroy() {
    ServiceFactoryManager<Demux>::UnregisterServiceFactory(&demux_);
  }

  bool UpdateRemoteServiceStatus(uint32_t id, uint32_t index,
                                 uint32_t error_code_value,
                                 Parameters parameters,
                                 boost::system::error_code& ec) {
    return p_service_manager_->update_remote(id, index, error_code_value,
                                             parameters, ec);
  }

  bool UpdateRemoteServiceStatus(uint32_t id, uint32_t error_code_value,
                                 boost::system::error_code& ec) {
    return p_service_manager_->update_remote(id, error_code_value, ec);
  }

  uint32_t GetIdFromParameters(uint32_t index, Parameters parameters) {
    return p_service_manager_->get_id(index, parameters);
  }

  uint32_t GetStatus(uint32_t id) { return p_service_manager_->get_status(id); }

  uint32_t GetStatus(uint32_t index, Parameters parameters, uint32_t id) {
    return p_service_manager_->get_status(index, parameters, id);
  }

 private:
  ServiceFactory(boost::asio::io_service& io_service, Demux& demux,
                 ServiceManagerPtr p_service_manager)
      : io_service_(io_service),
        demux_(demux),
        p_service_manager_(p_service_manager) {}

 private:
  boost::asio::io_service& io_service_;
  Demux& demux_;
  ServiceManagerPtr p_service_manager_;

  std::recursive_mutex service_creators_mutex_;
  ServiceCreatorMap service_creators_;
};

}  // ssf

#endif  // SSF_CORE_FACTORIES_SERVICE_FACTORY_H_

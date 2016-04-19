#ifndef SSF_CORE_SERVICE_MANAGER_SERVICE_MANAGER_H_
#define SSF_CORE_SERVICE_MANAGER_SERVICE_MANAGER_H_

#include <cstdint>

#include <map>
#include <utility>
#include <list>

#include <ssf/network/manager.h>

#include <boost/system/error_code.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include "services/base_service.h"

namespace ssf {

template <typename Demux>
class ServiceManager
    : public ItemManager<typename BaseService<Demux>::BaseServicePtr> {
 private:
  using Parameters = std::map<std::string, std::string> ;
  using ParametersIdPair = std::pair<Parameters, uint32_t>;
  using ServiceInstanceIdList = std::list<ParametersIdPair>;
  using ServiceTypeIdToInstancesListMap =
      std::map<uint32_t, ServiceInstanceIdList>;
  using IdToStatusMap = std::map<uint32_t, uint32_t>;
  using IdToServiceIdMap = std::map<uint32_t, uint32_t>;

 public:
  uint32_t get_id(uint32_t service_type_id, Parameters parameters) {
    boost::recursive_mutex::scoped_lock lock(status_n_instance_list_mutex_);

    auto& instance_list = intance_lists_[service_type_id];

    for (const auto& instance : instance_list) {
      if (instance.first == parameters) {
        return instance.second;
      }
    }

    return 0;
  }

  uint32_t find_error(uint32_t service_type_id, Parameters parameters) {
    boost::recursive_mutex::scoped_lock lock(status_n_instance_list_mutex_);

    auto& error_list = error_lists_[service_type_id];

    for (const auto& error : error_list) {
      if (error.first == parameters) {
        return error.second;
      }
    }

    return 0;
  }

  uint32_t get_status(uint32_t id) {
    if (status_.count(id)) {
      return status_[id];
    } else {
      return 0xFFFFFFFF;  // Undefined
    }
  }

  uint32_t get_status(uint32_t service_type_id, Parameters parameters,
                      uint32_t id) {
    if (status_.count(id)) {
      return status_[id];
    } else {
      auto error = find_error(service_type_id, parameters);
      if (error) {
        return error;
      } else {
        return 0xFFFFFFFF;  // Undefined
      }
    }
  }

  bool update_remote(uint32_t id, uint32_t service_type_id,
                     uint32_t error_code_value, Parameters parameters,
                     boost::system::error_code& ec) {
    boost::recursive_mutex::scoped_lock lock(status_n_instance_list_mutex_);

    if (!status_.count(id)) {
      add_remote(id, service_type_id, error_code_value, parameters);
      return true;
    } else {
      return update_remote(id, error_code_value, ec);
    }
  }

  bool update_remote(uint32_t id, uint32_t error_code_value,
                     boost::system::error_code& ec) {
    boost::recursive_mutex::scoped_lock lock(status_n_instance_list_mutex_);

    if (!status_.count(id)) {
      return false;
    } else {                        // check for value 4... stopping
      if (error_code_value == 4) {  // Service  stopped
        status_.erase(id);
        remove_id_from_instances(service_ids_[id], id);
        service_ids_.erase(id);
      } else {
        status_[id] = error_code_value;
      }
      return true;
    }
  }

 private:
  void add_remote(uint32_t id, uint32_t service_type_id,
                  uint32_t error_code_value, Parameters parameters) {
    boost::recursive_mutex::scoped_lock lock(status_n_instance_list_mutex_);

    if (id) {
      auto& instance_list = intance_lists_[service_type_id];
      instance_list.push_front(ParametersIdPair(parameters, id));
      service_ids_[id] = service_type_id;
      status_[id] = error_code_value;
    } else {
      auto& instance_list = error_lists_[service_type_id];
      instance_list.push_front(
          ParametersIdPair(parameters, error_code_value));
      if (instance_list.size() > 100) {
        instance_list.pop_back();
      }
    }
  }

  void remove_id_from_instances(uint32_t service_type_id, uint32_t id) {
    boost::recursive_mutex::scoped_lock lock(status_n_instance_list_mutex_);

    auto& instance_list = intance_lists_[service_type_id];

    for (auto it = instance_list.begin(); it != instance_list.end(); ++it) {
      if ((*it).second == id) {
        instance_list.erase(it);
        break;
      }
    }
  }

 private:
  boost::recursive_mutex status_n_instance_list_mutex_;

  IdToStatusMap status_;
  IdToServiceIdMap service_ids_;
  ServiceTypeIdToInstancesListMap intance_lists_;
  ServiceTypeIdToInstancesListMap error_lists_;
};

}  // ssf

#endif  // SSF_CORE_SERVICE_MANAGER_SERVICE_MANAGER_H_

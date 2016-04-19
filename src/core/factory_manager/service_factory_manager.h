#ifndef SSF_CORE_FACTORY_MANAGER_SERVICE_FACTORY_MANAGER_H_
#define SSF_CORE_FACTORY_MANAGER_SERVICE_FACTORY_MANAGER_H_

#include <map>
#include <memory>

#include <boost/thread/recursive_mutex.hpp>

namespace ssf {

template <typename Demux>
class ServiceFactory;

template <typename Demux>
class ServiceFactoryManager {
 private:
  using ServiceFactoryPtr = std::shared_ptr<ServiceFactory<Demux>>;
  using ServiceFactoryMap = std::map<Demux*, ServiceFactoryPtr>;

 public:
  static bool RegisterServiceFactory(Demux* index,
                                     ServiceFactoryPtr p_factory) {
    boost::recursive_mutex::scoped_lock lock(mutex_);
    auto inserted =
        service_factories_.insert(std::make_pair(index, std::move(p_factory)));
    return inserted.second;
  }

  static bool UnregisterServiceFactory(Demux* index) {
    boost::recursive_mutex::scoped_lock lock(mutex_);

    auto it = service_factories_.find(index);

    if (it != std::end(service_factories_)) {
      auto& p_service_factory = it->second;
      p_service_factory->StopAllLocalServices();
      service_factories_.erase(index);
      return true;
    } else {
      return false;
    }
  }

  static ServiceFactoryPtr GetServiceFactory(Demux* index) {
    boost::recursive_mutex::scoped_lock lock(mutex_);

    auto it = service_factories_.find(index);

    if (it != std::end(service_factories_)) {
      return it->second;
    } else {
      return nullptr;
    }
  }

 private:
  static boost::recursive_mutex mutex_;
  static ServiceFactoryMap service_factories_;
};

template <typename Demux>
boost::recursive_mutex ServiceFactoryManager<Demux>::mutex_;

template <typename Demux>
typename ServiceFactoryManager<Demux>::ServiceFactoryMap
    ServiceFactoryManager<Demux>::service_factories_;

}  // ssf

#endif  // SSF_CORE_FACTORY_MANAGER_SERVICE_FACTORY_MANAGER_H_

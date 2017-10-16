#ifndef SSF_SERVICES_USER_SERVICE_FACTORY_H_
#define SSF_SERVICES_USER_SERVICE_FACTORY_H_

#include <string>
#include <functional>
#include <map>
#include <memory>

#include <boost/system/error_code.hpp>

#include "common/error/error.h"
#include "services/user_services/base_user_service.h"
#include "services/user_services/parameters.h"

namespace ssf {

template <typename Demux>
class UserServiceFactory {
 public:
  using UserService = ssf::services::BaseUserService<Demux>;
  using UserServicePtr = std::shared_ptr<UserService>;

 private:
  using UserServiceGenerator =
      std::function<std::shared_ptr<ssf::services::BaseUserService<Demux>>(
          const UserServiceParameterBag&, boost::system::error_code&)>;

  using UserServiceGeneratorMap = std::map<std::string, UserServiceGenerator>;

 public:
  UserServiceFactory() {}

  template <class UserService>
  bool Register() {
    auto creator = [](const UserServiceParameterBag& parameters,
                      boost::system::error_code& ec) {
      return UserService::CreateUserService(parameters, ec);
    };
    return RegisterUserService(UserService::GetParseName(), creator);
  }

  bool RegisterUserService(const std::string& index,
                           UserServiceGenerator generator) {
    if (service_generators_.count(index)) {
      return false;
    }

    service_generators_[index] = generator;
    return true;
  }

  bool UnregisterUserService(const std::string& index) {
    auto it = service_generators_.find(index);

    if (it != std::end(service_generators_)) {
      service_generators_.erase(it);
      return true;
    } else {
      return false;
    }
  }

  UserServicePtr CreateUserService(
      const std::string& service_name,
      const UserServiceParameterBag& service_params,
      boost::system::error_code& ec) const {
    auto it = service_generators_.find(service_name);

    if (it != std::end(service_generators_)) {
      return it->second(service_params, ec);
    } else {
      ec.assign(::error::service_not_found, ::error::get_ssf_category());
      return UserServicePtr(nullptr);
    }
  }

 private:
  UserServiceGeneratorMap service_generators_;
};

}  // ssf

#endif  // SSF_SERVICES_USER_SERVICE_FACTORY_H_

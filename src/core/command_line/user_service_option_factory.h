#ifndef SSF_CORE_COMMAND_LINE_USER_SERVICE_OPTION_FACTORY_H_
#define SSF_CORE_COMMAND_LINE_USER_SERVICE_OPTION_FACTORY_H_

#include <functional>
#include <map>
#include <string>

#include <boost/program_options.hpp>
#include <boost/system/error_code.hpp>

#include "common/error/error.h"
#include "services/user_services/parameters.h"

namespace ssf {

class UserServiceOptionFactory {
  using OptionParser = std::function<UserServiceParameterBag(
      const std::string&, boost::system::error_code&)>;

  struct ServiceOption {
    std::string fullname;
    std::string value_name;
    std::string description;
    OptionParser parser;
  };

  using ServiceOptionMap = std::map<std::string, ServiceOption>;

 public:
  UserServiceOptionFactory() {}

  template <class UserService>
  bool Register() {
    return RegisterUserService(
        UserService::GetParseName(), UserService::GetFullParseName(),
        UserService::GetValueName(), UserService::GetParseDesc(),
        &UserService::CreateUserServiceParameters);
  }

  bool RegisterUserService(const std::string& index,
                           const std::string& full_name,
                           const std::string& value_name,
                           const std::string& description,
                           OptionParser parser) {
    if (service_options_.count(index)) {
      return false;
    }

    service_options_[index] = {full_name, value_name, description, parser};
    return true;
  }

  bool UnregisterUserService(const std::string& index) {
    auto it = service_options_.find(index);

    if (it != std::end(service_options_)) {
      service_options_.erase(it);
      return true;
    } else {
      return false;
    }
  }

  bool HasService(const std::string& service_id) const {
    return service_options_.count(service_id) > 0;
  }

  boost::program_options::options_description GetOptions() const {
    boost::program_options::options_description desc(
        "Supported service commands");

    for (const auto& option : service_options_) {
      desc.add_options()(
          option.second.fullname.c_str(),
          boost::program_options::value<std::vector<std::string>>()->value_name(
              option.second.value_name.c_str()),
          option.second.description.c_str());
    }

    return desc;
  }

  UserServiceParameterBag CreateUserServiceParameters(
      const std::string& index, const std::string& service_param,
      boost::system::error_code& ec) const {
    auto it = service_options_.find(index);

    if (it == std::end(service_options_)) {
      ec.assign(::error::service_not_found, ::error::get_ssf_category());
      return {};
    }

    return it->second.parser(service_param, ec);
  }

 private:
  ServiceOptionMap service_options_;
};

}  // ssf

#endif  // SSF_CORE_COMMAND_LINE_USER_SERVICE_OPTION_FACTORY_H_

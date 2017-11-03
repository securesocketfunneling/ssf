#ifndef SSF_CORE_COMMAND_LINE_USER_SERVICE_OPTION_FACTORY_H_
#define SSF_CORE_COMMAND_LINE_USER_SERVICE_OPTION_FACTORY_H_

#include <functional>
#include <map>
#include <string>

#include <boost/program_options.hpp>
#include <boost/system/error_code.hpp>

#include "common/error/error.h"
#include "services/user_services/parameters.h"

#include "core/command_line/base.h"

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
        UserService::GetParseName(),
        UserService::GetFullParseName(),
        UserService::GetValueName(),
        UserService::GetParseDesc(),
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

  UserServiceParameters CreateUserServiceParameters(
      const ssf::command_line::Base::Options& opts,
      boost::system::error_code& ec) const {
    UserServiceParameters result;

    for (const auto& service: service_options_) {
        const std::string& option = service.first;
        auto& params = opts[option].as<std::vector<std::string>>();

        for (const auto& line: params) {
          result[option].emplace_back(service.second.parser(line, ec));
          if (ec) {
            return {};
          }
        }
    }

    return result;
  }

  void InitOptions(ssf::command_line::Base::Options& opts) const {
    for (const auto& it: service_options_)
      opts.add_options("Services")
        (it.second.fullname, it.second.description,
            cxxopts::value<std::vector<std::string>>(),
            it.second.value_name);
  }

 private:
  ServiceOptionMap service_options_;
};

}  // ssf

#endif  // SSF_CORE_COMMAND_LINE_USER_SERVICE_OPTION_FACTORY_H_

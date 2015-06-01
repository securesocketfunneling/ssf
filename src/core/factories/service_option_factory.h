#ifndef SSF_CORE_FACTORIES_SERVICE_OPTION_FACTORY_H_
#define SSF_CORE_FACTORIES_SERVICE_OPTION_FACTORY_H_

#include <string>
#include <functional>
#include <map>

#include <boost/thread/recursive_mutex.hpp>

#include <boost/program_options.hpp>

#include <boost/system/error_code.hpp>

#include "common/error/error.h"
#include "services/user_services/base_user_service.h"

namespace ssf {
template <typename Demux>
class ServiceOptionFactory {
private:
  typedef std::function<std::shared_ptr<ssf::services::BaseUserService<Demux>>(const std::string&,
                                                                               boost::system::error_code&)>
    ServiceParserType;

  struct ParserDescriptor {
    ServiceParserType parser;
    std::string fullname;
    std::string description;
  };

  typedef std::map<std::string, ParserDescriptor> ServiceParserMap;

public:
 static bool RegisterUserServiceParser(std::string index, std::string full_name,
                                       std::string description,
                                       ServiceParserType parser) {
    boost::recursive_mutex::scoped_lock lock(service_options_mutex_);
    if (service_options_.count(index)) {
      return false;
    } else {
      service_options_[index] = {std::move(parser), std::move(full_name),
                                 std::move(description)};
      return true;
    }
  }

 static bool UnregisterUserServiceParser(std::string index) {
   boost::recursive_mutex::scoped_lock lock(service_options_mutex_);
   auto it = service_options_.find(index);

   if (it != std::end(service_options_)) {
     service_options_.erase(it);
     return true;
   } else {
     return false;
   }
 }

  static boost::program_options::options_description GetOptionDescriptions() {
    boost::program_options::options_description desc("Supported service commands");

    boost::recursive_mutex::scoped_lock lock(service_options_mutex_);
    for (auto& option : service_options_) {
      desc.add_options()(option.second.fullname.c_str(),
                         boost::program_options::value<std::vector<std::string>>(),
                         option.second.description.c_str());
    }

    return desc;
  }

  static std::shared_ptr<ssf::services::BaseUserService<Demux>> ParseServiceLine(std::string option,
                                                 std::string parameters,
                                                 boost::system::error_code& ec) {
    boost::recursive_mutex::scoped_lock lock(service_options_mutex_);
    auto it = service_options_.find(option);

    if (it != std::end(service_options_)) {
      return it->second.parser(parameters, ec);
    } else {
      ec.assign(ssf::error::service_not_found, ssf::error::get_ssf_category());
      return std::shared_ptr<ssf::services::BaseUserService<Demux>>(nullptr);
    }
  }

private:
  static boost::recursive_mutex service_options_mutex_;
  static ServiceParserMap service_options_;
};

template <typename Demux>
boost::recursive_mutex ServiceOptionFactory<Demux>::service_options_mutex_;

template <typename Demux>
typename ServiceOptionFactory<Demux>::ServiceParserMap ServiceOptionFactory<Demux>::service_options_;

}  // ssf


#endif  // SSF_CORE_FACTORIES_SERVICE_OPTION_FACTORY_H_
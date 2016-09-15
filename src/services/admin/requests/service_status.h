#ifndef SSF_SERVICES_ADMIN_REQUESTS_SERVICE_STATUS_H_
#define SSF_SERVICES_ADMIN_REQUESTS_SERVICE_STATUS_H_

#include <cstdint>

#include <map>
#include <fstream>
#include <string>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/map.hpp>
#include <boost/system/error_code.hpp>

#include <ssf/log/log.h>

#include "core/factories/command_factory.h"
#include "core/factories/service_factory.h"

#include "core/factory_manager/service_factory_manager.h"

namespace ssf {
namespace services {
namespace admin {

template <typename Demux>
class ServiceStatus {
 private:
  typedef std::map<std::string, std::string> Parameters;

 public:
  ServiceStatus() {}

  ServiceStatus(uint32_t id, uint32_t service_id, uint32_t error_code_value,
                Parameters parameters)
      : id_(id),
        service_id_(service_id),
        error_code_value_(error_code_value),
        parameters_(parameters) {}

  enum { command_id = 2, reply_id = 2 };

  static void RegisterToCommandFactory() {
    CommandFactory<Demux>::RegisterOnReceiveCommand(command_id,
                                                    &ServiceStatus::OnReceive);
    CommandFactory<Demux>::RegisterOnReplyCommand(command_id,
                                                  &ServiceStatus::OnReply);
    CommandFactory<Demux>::RegisterReplyCommandIndex(command_id, reply_id);
  }

  static std::string OnReceive(boost::archive::text_iarchive& ar,
                               Demux* p_demux, boost::system::error_code& ec) {
    ServiceStatus<Demux> status;

    try {
      ar >> status;
    } catch (const std::exception&) {
      return std::string();
    }

    auto p_service_factory =
        ServiceFactoryManager<Demux>::GetServiceFactory(p_demux);

    if (status.service_id()) {
      p_service_factory->UpdateRemoteServiceStatus(
          status.id(), status.service_id(), status.error_code_value(),
          status.parameters(), ec);
    } else {
      p_service_factory->UpdateRemoteServiceStatus(
          status.id(), status.error_code_value(), ec);
    }

    SSF_LOG(kLogDebug) << "service status: received "
                       << "service unique id " << status.id() << " service id "
                       << status.service_id() << " - error_code "
                       << status.error_code_value();

    return std::string();
  }

  static std::string OnReply(boost::archive::text_iarchive& ar, Demux* p_demux,
                             const boost::system::error_code& ec,
                             std::string serialized_result) {
    return std::string();
  }

  std::string OnSending() const {
    std::ostringstream ostrs;
    boost::archive::text_oarchive ar(ostrs);

    ar << *this;

    return ostrs.str();
  }

  uint32_t id() { return id_; }

  uint32_t service_id() { return service_id_; }

  Parameters parameters() { return parameters_; }

  uint32_t error_code_value() { return error_code_value_; }

  void add_parameter(const std::string& key, const std::string& value) {
    parameters_[key] = value;
  }

 private:
  friend class boost::serialization::access;

  template <typename Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& id_;
    ar& service_id_;
    ar& error_code_value_;
    ar& BOOST_SERIALIZATION_NVP(parameters_);
  }

 private:
  uint32_t id_;
  uint32_t service_id_;
  uint32_t error_code_value_;
  Parameters parameters_;
};

}  // admin
}  // services
}  // ssf

#endif  // SSF_SERVICES_ADMIN_REQUESTS_SERVICE_STATUS_H_
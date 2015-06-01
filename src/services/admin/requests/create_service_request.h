#ifndef SSF_SERVICES_ADMIN_REQUESTS_CREATE_SERVICE_REQUEST_H_
#define SSF_SERVICES_ADMIN_REQUESTS_CREATE_SERVICE_REQUEST_H_

#include <cstdint>

#include <map>
#include <fstream>
#include <sstream>
#include <string>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/map.hpp>
#include <boost/system/error_code.hpp>

#include "core/factories/command_factory.h"
#include "core/factories/service_factory.h"

#include "core/factory_manager/service_factory_manager.h"


#include "services/admin/requests/service_status.h"

namespace ssf { namespace services { namespace admin {

template <typename Demux>
class CreateServiceRequest {
private:
  typedef std::map<std::string, std::string> Parameters;
public:
  CreateServiceRequest() {}

  CreateServiceRequest(uint32_t service_id) 
    : service_id_(service_id) {}

  CreateServiceRequest(uint32_t service_id, Parameters parameters)
    : service_id_(service_id), parameters_(parameters) {}

  enum {
    command_id = 1,
    reply_id = 2
  };
  
  static void RegisterToCommandFactory() {
    CommandFactory<Demux>::RegisterOnReceiveCommand(command_id,
                                                    &CreateServiceRequest::OnReceive);
    CommandFactory<Demux>::RegisterOnReplyCommand(command_id,
                                                  &CreateServiceRequest::OnReply);
    CommandFactory<Demux>::RegisterReplyCommandIndex(command_id, reply_id);
  }

  static std::string OnReceive(boost::archive::text_iarchive& ar, 
                        Demux* p_demux,
                        boost::system::error_code& ec) {
    CreateServiceRequest<Demux> request;

    try {
      ar >> request;
    } catch (const std::exception&) {
      return std::string();
    }

    auto p_service_factory = 
      ServiceFactoryManager<Demux>::GetServiceFactory(p_demux);

    auto id = p_service_factory->CreateRunNewService(request.service_id(),
                                           request.parameters(),
                                           ec);

    BOOST_LOG_TRIVIAL(debug) << "service status: create "
      << "service unique id " << id
      << " - error_code " << ec.value();

    std::stringstream ss;
    std::string result;

    ss << id;
    ss >> result;

    return result;
  }

  static std::string OnReply(boost::archive::text_iarchive& ar,
                             Demux* p_demux,
                             const boost::system::error_code& ec,
                             std::string serialized_result) {
    CreateServiceRequest<Demux> request;

    try {
    ar >> request;
    } catch (const std::exception&) {
      return std::string();
    }

    ServiceStatus<Demux> reply(std::stoul(serialized_result),
                               request.service_id(), 
                               ec.value(), 
                               request.parameters());

    return reply.OnSending();
  }

  std::string OnSending() const {
    std::ostringstream ostrs;
    boost::archive::text_oarchive ar(ostrs);

    ar << *this;

    return ostrs.str();
  }

  uint32_t service_id() {
    return service_id_;
  }

  Parameters parameters() {
    return parameters_;
  }

  void add_parameter(const std::string& key, const std::string& value) {
    parameters_[key] = value;
  }

private:
  friend class boost::serialization::access;

  template <typename Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & service_id_;
    ar & BOOST_SERIALIZATION_NVP(parameters_);
  }

private:
  uint32_t service_id_;
  Parameters parameters_;
};

}  // admin
}  // services
}  // ssf


#endif  // SSF_SERVICES_ADMIN_REQUESTS_CREATE_SERVICE_REQUEST_H_
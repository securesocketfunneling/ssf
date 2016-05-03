#ifndef SSF_SERVICES_ADMIN_REQUESTS_STOP_SERVICE_REQUEST_H_
#define SSF_SERVICES_ADMIN_REQUESTS_STOP_SERVICE_REQUEST_H_

#include <cstdint>

#include <map>
#include <fstream>
#include <sstream>
#include <string>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/map.hpp>
#include <boost/system/error_code.hpp>

#include <ssf/log/log.h>

#include "core/factories/command_factory.h"
#include "core/factories/service_factory.h"

#include "core/factory_manager/service_factory_manager.h"

#include "services/admin/requests/service_status.h"

namespace ssf {
namespace services {
namespace admin {

template <typename Demux>
class StopServiceRequest {
 private:
  typedef std::map<std::string, std::string> Parameters;

 public:
  StopServiceRequest() {}

  StopServiceRequest(uint32_t unique_id) : unique_id_(unique_id) {}

  enum { command_id = 3, reply_id = 2 };

  static void RegisterToCommandFactory() {
    CommandFactory<Demux>::RegisterOnReceiveCommand(
        command_id, &StopServiceRequest::OnReceive);
    CommandFactory<Demux>::RegisterOnReplyCommand(command_id,
                                                  &StopServiceRequest::OnReply);
    CommandFactory<Demux>::RegisterReplyCommandIndex(command_id, reply_id);
  }

  static std::string OnReceive(boost::archive::text_iarchive& ar,
                               Demux* p_demux, boost::system::error_code& ec) {
    StopServiceRequest<Demux> request;

    try {
      ar >> request;
    } catch (const std::exception&) {
      return std::string();
    }

    auto p_service_factory =
        ServiceFactoryManager<Demux>::GetServiceFactory(p_demux);

    p_service_factory->StopService(request.unique_id());

    SSF_LOG(kLogDebug) << "service status: stop request";

    ec.assign(boost::system::errc::interrupted,
              boost::system::system_category());

    std::stringstream ss;
    std::string result;

    ss << request.unique_id();
    ss >> result;

    return result;
  }

  static std::string OnReply(boost::archive::text_iarchive& ar, Demux* p_demux,
                             const boost::system::error_code& ec,
                             std::string serialized_result) {
    StopServiceRequest<Demux> request;

    try {
      ar >> request;
    } catch (const std::exception&) {
      return std::string();
    }

    ServiceStatus<Demux> reply(std::stoul(serialized_result), 0, ec.value(),
                               Parameters());

    return reply.OnSending();
  }

  std::string OnSending() const {
    std::ostringstream ostrs;
    boost::archive::text_oarchive ar(ostrs);

    ar << *this;

    return ostrs.str();
  }

  uint32_t unique_id() { return unique_id_; }

 private:
  friend class boost::serialization::access;

  template <typename Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& unique_id_;
  }

 private:
  uint32_t unique_id_;
};

}  // admin
}  // services
}  // ssf

#endif  // SSF_SERVICES_ADMIN_REQUESTS_STOP_SERVICE_REQUEST_H_
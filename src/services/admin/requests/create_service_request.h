#ifndef SSF_SERVICES_ADMIN_REQUESTS_CREATE_SERVICE_REQUEST_H_
#define SSF_SERVICES_ADMIN_REQUESTS_CREATE_SERVICE_REQUEST_H_

#include <cstdint>

#include <fstream>
#include <map>
#include <sstream>
#include <string>

#include <boost/system/error_code.hpp>

#include <msgpack.hpp>

#include <ssf/log/log.h>

#include "core/factories/service_factory.h"

#include "core/factory_manager/service_factory_manager.h"

#include "services/admin/command_factory.h"
#include "services/admin/requests/service_status.h"

namespace ssf {
namespace services {
namespace admin {

template <typename Demux>
class CreateServiceRequest {
 private:
  typedef std::map<std::string, std::string> Parameters;

 public:
  CreateServiceRequest() {}

  CreateServiceRequest(uint32_t service_id) : service_id_(service_id) {}

  CreateServiceRequest(uint32_t service_id, Parameters parameters)
      : service_id_(service_id), parameters_(parameters) {}

  enum { command_id = 1, reply_id = 2 };

  static bool RegisterOnReceiveCommand(CommandFactory<Demux>* cmd_factory) {
    return cmd_factory->RegisterOnReceiveCommand(
        command_id, &CreateServiceRequest::OnReceive);
  }

  static bool RegisterOnReplyCommand(CommandFactory<Demux>* cmd_factory) {
    return cmd_factory->RegisterOnReplyCommand(command_id,
                                               &CreateServiceRequest::OnReply);
  }

  static bool RegisterReplyCommandIndex(CommandFactory<Demux>* cmd_factory) {
    return cmd_factory->RegisterReplyCommandIndex(command_id, reply_id);
  }

  static std::string OnReceive(const std::string& serialized_request,
                               Demux* p_demux, boost::system::error_code& ec) {
    CreateServiceRequest<Demux> request;

    try {
      auto obj_handle =
          msgpack::unpack(serialized_request.data(), serialized_request.size());
      auto obj = obj_handle.get();
      obj.convert(request);
    } catch (const std::exception&) {
      SSF_LOG("microservice", warn,
              "[admin] create service[on receive]: cannot extract request");
      ec.assign(::error::invalid_argument, ::error::get_ssf_category());
      return {};
    }

    auto p_service_factory =
        ServiceFactoryManager<Demux>::GetServiceFactory(p_demux);

    auto id = p_service_factory->CreateRunNewService(request.service_id(),
                                                     request.parameters(), ec);

    SSF_LOG("microservice", debug, "[admin] create service: {} - ec {}", id,
            ec.value());

    std::stringstream ss;
    std::string result;

    ss << id;
    ss >> result;

    return result;
  }

  static std::string OnReply(const std::string& serialized_request,
                             Demux* p_demux,
                             const boost::system::error_code& ec,
                             std::string serialized_result) {
    CreateServiceRequest<Demux> request;

    try {
      auto obj_handle =
          msgpack::unpack(serialized_request.data(), serialized_request.size());
      auto obj = obj_handle.get();
      obj.convert(request);
    } catch (const std::exception&) {
      SSF_LOG("microservice", warn,
              "[admin] create service[on reply]: cannot extract request");
      return {};
    }

    uint32_t id;
    try {
      id = std::stoul(serialized_result);
    } catch (const std::exception&) {
      // TODO: ec?
      SSF_LOG("microservice", warn,
              "[admin] create service request: extract reply id failed");
      return {};
    }

    ServiceStatus<Demux> reply(id, request.service_id(), ec.value(),
                               request.parameters());

    return reply.OnSending();
  }

  std::string OnSending() const {
    std::ostringstream ostrs;
    msgpack::pack(ostrs, *this);
    return ostrs.str();
  }

  uint32_t service_id() { return service_id_; }

  Parameters parameters() { return parameters_; }

  void add_parameter(const std::string& key, const std::string& value) {
    parameters_[key] = value;
  }

 public:
  // add msgpack function definitions
  MSGPACK_DEFINE(service_id_, parameters_)

 private:
  uint32_t service_id_;
  Parameters parameters_;
};

}  // admin
}  // services
}  // ssf

#endif  // SSF_SERVICES_ADMIN_REQUESTS_CREATE_SERVICE_REQUEST_H_

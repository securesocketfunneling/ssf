#ifndef SSF_SERVICES_ADMIN_REQUESTS_SERVICE_STATUS_H_
#define SSF_SERVICES_ADMIN_REQUESTS_SERVICE_STATUS_H_

#include <cstdint>

#include <fstream>
#include <map>
#include <string>

#include <boost/system/error_code.hpp>

#include <msgpack.hpp>

#include <ssf/log/log.h>

#include "core/factories/service_factory.h"

#include "core/factory_manager/service_factory_manager.h"
#include "services/admin/command_factory.h"

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

  static bool RegisterOnReceiveCommand(CommandFactory<Demux>* cmd_factory) {
    return cmd_factory->RegisterOnReceiveCommand(command_id,
                                                 &ServiceStatus::OnReceive);
  }

  static bool RegisterOnReplyCommand(CommandFactory<Demux>* cmd_factory) {
    return cmd_factory->RegisterOnReplyCommand(command_id,
                                               &ServiceStatus::OnReply);
  }

  static bool RegisterReplyCommandIndex(CommandFactory<Demux>* cmd_factory) {
    return cmd_factory->RegisterReplyCommandIndex(command_id, reply_id);
  }

  static std::string OnReceive(const std::string& serialized_request,
                               Demux* p_demux, boost::system::error_code& ec) {
    ServiceStatus<Demux> status;

    try {
      auto obj_handle =
          msgpack::unpack(serialized_request.data(), serialized_request.size());
      auto obj = obj_handle.get();
      obj.convert(status);
    } catch (const std::exception&) {
      SSF_LOG("microservice", warn,
              "[admin] service status[on receive]: cannot extract request");
      ec.assign(::error::invalid_argument, ::error::get_ssf_category());
      return {};
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

    SSF_LOG("microservice", debug,
            "[admin] service status: received {} for service {} - ec {} ",
            status.id(), status.service_id(), status.error_code_value());

    return {};
  }

  static std::string OnReply(const std::string& serialized_request,
                             Demux* p_demux,
                             const boost::system::error_code& ec,
                             const std::string& serialized_result) {
    if (ec) {
      SSF_LOG("microservice", warn, "[admin] service status[on reply] error");
      return {};
    }

    return {};
  }

  std::string OnSending() const {
    std::ostringstream ostrs;
    msgpack::pack(ostrs, *this);
    return ostrs.str();
  }

  uint32_t id() { return id_; }

  uint32_t service_id() { return service_id_; }

  Parameters parameters() { return parameters_; }

  uint32_t error_code_value() { return error_code_value_; }

  void add_parameter(const std::string& key, const std::string& value) {
    parameters_[key] = value;
  }

 public:
  // add msgpack function definitions
  MSGPACK_DEFINE(id_, service_id_, error_code_value_, parameters_)

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

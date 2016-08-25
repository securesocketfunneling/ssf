#ifndef SSF_SERVICES_USER_SERVICES_COPY_FILE_SERVICE_H_
#define SSF_SERVICES_USER_SERVICES_COPY_FILE_SERVICE_H_

#include <cstdint>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <boost/program_options.hpp>

#include "services/user_services/base_user_service.h"
#include "services/copy_file/file_enquirer/file_enquirer.h"
#include "services/copy_file/file_to_fiber/file_to_fiber.h"
#include "services/copy_file/fiber_to_file/fiber_to_file.h"

namespace ssf {
namespace services {

template <typename Demux>
class CopyFileService : public BaseUserService<Demux> {
 public:
  using ServiceId = uint32_t;
  using ServideIdSet = std::set<ServiceId>;
  using UserParameters = std::map<std::string, std::vector<std::string>>;
  using CopyFileServicePtr = std::shared_ptr<CopyFileService>;

 public:
  // Create user service from parameters (factory method)
  static std::shared_ptr<CopyFileService> CreateServiceFromParams(
      bool from_stdin, bool from_local_to_remote,
      const std::string& input_pattern, const std::string& output_pattern,
      boost::system::error_code& ec) {
    ec.assign(::error::success, ::error::get_ssf_category());
    return CopyFileServicePtr(new CopyFileService(
        from_stdin, from_local_to_remote, input_pattern, output_pattern));
  }

  virtual std::string GetName() { return "copy_file"; }

  virtual std::vector<admin::CreateServiceRequest<Demux>>
  GetRemoteServiceCreateVector() {
    std::vector<admin::CreateServiceRequest<Demux>> result;
    result.push_back(GetRemoteServiceRequest());
    return result;
  }

  virtual std::vector<admin::StopServiceRequest<Demux>>
  GetRemoteServiceStopVector(Demux& demux) {
    std::vector<admin::StopServiceRequest<Demux>> result;

    return result;
  }

  virtual uint32_t CheckRemoteServiceStatus(Demux& demux) {
    services::admin::CreateServiceRequest<Demux> remote_service_request =
        GetRemoteServiceRequest();

    auto p_service_factory =
        ServiceFactoryManager<Demux>::GetServiceFactory(&demux);
    auto status = p_service_factory->GetStatus(
        remote_service_request.service_id(),
        remote_service_request.parameters(), GetRemoteServiceId(demux));

    return status;
  }

  virtual bool StartLocalServices(Demux& demux) {
    auto p_service_factory =
        ServiceFactoryManager<Demux>::GetServiceFactory(&demux);
    boost::system::error_code ec;

    if (from_local_to_remote_) {
      // Local to remote : file to fiber only
      services::admin::CreateServiceRequest<Demux> local_service_request;
      local_service_request = services::copy_file::file_to_fiber::FileToFiber<
          Demux>::GetCreateRequest(false, from_stdin_, input_pattern_,
                                   output_pattern_);

      local_service_ids_.insert(p_service_factory->CreateRunNewService(
          local_service_request.service_id(),
          local_service_request.parameters(), ec));

      if (ec) {
        SSF_LOG(kLogError) << "user_service[copy file]: "
                           << "local_service[file to fibers]: start failed: "
                           << ec.message();
      }
      return !ec;
    }

    // Remote to local : fiber to file, file enquirer
    services::admin::CreateServiceRequest<Demux> fiber_to_file_request =
        services::copy_file::fiber_to_file::FiberToFile<
            Demux>::GetCreateRequest();

    services::admin::CreateServiceRequest<Demux> file_enquirer_request =
        services::copy_file::file_enquirer::FileEnquirer<
            Demux>::GetCreateRequest(input_pattern_, output_pattern_);

    local_service_ids_.insert(p_service_factory->CreateRunNewService(
        fiber_to_file_request.service_id(), fiber_to_file_request.parameters(),
        ec));
    if (ec) {
      SSF_LOG(kLogError) << "user_service[copy file]: "
                         << "local_service[fiber to file]: start failed: "
                         << ec.message();
    }

    local_service_ids_.insert(p_service_factory->CreateRunNewService(
        file_enquirer_request.service_id(), file_enquirer_request.parameters(),
        ec));
    if (ec) {
      SSF_LOG(kLogError) << "user_service[copy file]: "
                         << "local_service[file enquirer]: start failed: "
                         << ec.message();
    }

    return !ec;
  }

  virtual void StopLocalServices(Demux& demux) {
    auto p_service_factory =
        ServiceFactoryManager<Demux>::GetServiceFactory(&demux);
    for (auto& local_service_id : local_service_ids_) {
      p_service_factory->StopService(local_service_id);
    }
    local_service_ids_.clear();
  }

  virtual ~CopyFileService() {}

 private:
  CopyFileService(bool from_stdin, bool from_local_to_remote,
                  const std::string& input_pattern,
                  const std::string& output_pattern)
      : from_stdin_(from_stdin),
        from_local_to_remote_(from_local_to_remote || from_stdin),
        input_pattern_(input_pattern),
        output_pattern_(output_pattern),
        remote_service_id_(0),
        local_service_ids_() {}

  services::admin::CreateServiceRequest<Demux> GetRemoteServiceRequest() {
    services::admin::CreateServiceRequest<Demux> remote_service_request;
    if (from_local_to_remote_) {
      remote_service_request = services::copy_file::fiber_to_file::FiberToFile<
          Demux>::GetCreateRequest();
    } else {
      remote_service_request = services::copy_file::file_to_fiber::FileToFiber<
          Demux>::GetCreateRequest(true, false, "", "");
    }
    return remote_service_request;
  }

  services::admin::CreateServiceRequest<Demux> GetLocalServiceRequest() {
    services::admin::CreateServiceRequest<Demux> local_service_request;
    if (from_local_to_remote_) {
      local_service_request = services::copy_file::file_to_fiber::FileToFiber<
          Demux>::GetCreateRequest(false, from_stdin_, input_pattern_,
                                   output_pattern_);
    } else {
      local_service_request = services::copy_file::fiber_to_file::FiberToFile<
          Demux>::GetCreateRequest();
    }
    return local_service_request;
  }

  uint32_t GetRemoteServiceId(Demux& demux) {
    if (remote_service_id_) {
      return remote_service_id_;
    } else {
      auto remote_service_request = GetRemoteServiceRequest();

      auto p_service_factory =
          ServiceFactoryManager<Demux>::GetServiceFactory(&demux);
      auto id = p_service_factory->GetIdFromParameters(
          remote_service_request.service_id(),
          remote_service_request.parameters());
      remote_service_id_ = id;
      return id;
    }
  }

 private:
  bool from_stdin_;
  bool from_local_to_remote_;
  std::string input_pattern_;
  std::string output_pattern_;
  ServiceId remote_service_id_;
  ServideIdSet local_service_ids_;
};

}  // services
}  // ssf

#endif  // SSF_SERVICES_USER_SERVICES_COPY_FILE_SERVICE_H_

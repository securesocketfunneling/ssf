#ifndef SSF_SERVICES_COPY_FILE_FILE_ENQUIRER_FILE_ENQUIRER_H_
#define SSF_SERVICES_COPY_FILE_FILE_ENQUIRER_FILE_ENQUIRER_H_

#include <functional>

#include <boost/asio/write.hpp>

#include <ssf/log/log.h>

#include "core/factories/service_factory.h"
#include "services/admin/requests/create_service_request.h"
#include "services/copy_file/config.h"
#include "services/copy_file/file_to_fiber/file_to_fiber.h"

namespace ssf {
namespace services {
namespace copy_file {
namespace file_enquirer {

template <class Demux>
class FileEnquirer : public BaseService<Demux> {
 private:
  using Endpoint = typename ssf::BaseService<Demux>::endpoint;
  using FileEnquirerPtr = std::shared_ptr<FileEnquirer>;
  using FiberPort = typename Demux::local_port_type;
  using Fiber = typename ssf::BaseService<Demux>::fiber;
  using FiberPtr = std::shared_ptr<Fiber>;
  using FilenameBuffer = ssf::services::copy_file::FilenameBuffer;
  using Parameters = typename ssf::BaseService<Demux>::Parameters;

 public:
  enum { kFactoryId = 9 };

  static FileEnquirerPtr Create(boost::asio::io_service& io_service,
                                Demux& fiber_demux,
                                const Parameters& parameters) {
    if (parameters.count("input_pattern") != 0 &&
        parameters.count("output_pattern") != 0) {
      return FileEnquirerPtr(new FileEnquirer(io_service, fiber_demux,
                                              parameters.at("input_pattern"),
                                              parameters.at("output_pattern")));
    }

    return nullptr;
  }

  static void RegisterToServiceFactory(
      std::shared_ptr<ServiceFactory<Demux>> p_factory, const Config& config) {
    if (!config.enabled()) {
      // service factory is not enabled
      return;
    }
    auto creator = [](boost::asio::io_service& io_service, demux& fiber_demux,
                      const Parameters& parameters) {
      return FileEnquirer::Create(io_service, fiber_demux, parameters);
    };
    p_factory->RegisterServiceCreator(kFactoryId, creator);
  }

  static ssf::services::admin::CreateServiceRequest<Demux> GetCreateRequest(
      const std::string& input_pattern, const std::string& output_pattern) {
    ssf::services::admin::CreateServiceRequest<Demux> create_req(kFactoryId);
    create_req.add_parameter("input_pattern", input_pattern);
    create_req.add_parameter("output_pattern", output_pattern);

    return create_req;
  }

 public:
  void start(boost::system::error_code& ec) override {
    auto self = this->shared_from_this();
    auto on_fiber_connect = [this, self](const boost::system::error_code& ec) {
      OnFiberConnect(ec);
    };
    fiber_.async_connect(remote_endpoint_, std::move(on_fiber_connect));
  }

  void stop(boost::system::error_code& ec) override { fiber_.close(ec); }

  uint32_t service_type_id() override { return kFactoryId; }

 private:
  FileEnquirer(boost::asio::io_service& io_service, Demux& fiber_demux,
               const std::string& input_pattern,
               const std::string& output_pattern)
      : BaseService<Demux>(io_service, fiber_demux),
        fiber_(io_service),
        input_request_(input_pattern),
        output_request_(output_pattern),
        remote_endpoint_(fiber_demux,
                         ssf::services::copy_file::file_to_fiber::FileToFiber<
                             Demux>::kServicePort) {}

  void OnFiberConnect(const boost::system::error_code& ec) {
    if (ec) {
      boost::system::error_code close_ec;
      fiber_.close(close_ec);
      return;
    }

    SSF_LOG(kLogDebug) << "microservice[file enquirer]: connect to remote "
                          "fiber acceptor port "
                       << remote_endpoint_.port();
    SendRequest(ec, 0);
  }

#include <boost/asio/yield.hpp>
  // Connect to remote file to fiber service and send input and output pattern
  void SendRequest(const boost::system::error_code& ec, std::size_t length) {
    if (ec) {
      boost::system::error_code close_ec;
      fiber_.close(close_ec);
      return;
    }

    reenter(coroutine_) {
      // write input pattern size from request
      yield boost::asio::async_write(
          fiber_, input_request_.GetFilenameSizeConstBuffers(),
          std::bind(&FileEnquirer<Demux>::SendRequest, this->SelfFromThis(),
                    std::placeholders::_1, std::placeholders::_2));

      // write input pattern from request
      yield boost::asio::async_write(
          fiber_, input_request_.GetFilenameConstBuffers(),
          std::bind(&FileEnquirer<Demux>::SendRequest, this->SelfFromThis(),
                    std::placeholders::_1, std::placeholders::_2));

      // write output pattern size from request
      yield boost::asio::async_write(
          fiber_, output_request_.GetFilenameSizeConstBuffers(),
          std::bind(&FileEnquirer<Demux>::SendRequest, this->SelfFromThis(),
                    std::placeholders::_1, std::placeholders::_2));

      // write output pattern from request
      yield boost::asio::async_write(
          fiber_, output_request_.GetFilenameConstBuffers(),
          std::bind(&FileEnquirer<Demux>::SendRequest, this->SelfFromThis(),
                    std::placeholders::_1, std::placeholders::_2));
    }
  }
#include <boost/asio/unyield.hpp>

  FileEnquirerPtr SelfFromThis() {
    return std::static_pointer_cast<FileEnquirer>(this->shared_from_this());
  }

 private:
  Fiber fiber_;

  // coroutine variables
  boost::asio::coroutine coroutine_;
  FilenameBuffer input_request_;
  FilenameBuffer output_request_;
  Endpoint remote_endpoint_;
};

}  // file_enquirer
}  // copy_file
}  // services
}  // ssf

#endif  // SSF_SERVICES_COPY_FILE_FILE_ENQUIRER_FILE_ENQUIRER_H_

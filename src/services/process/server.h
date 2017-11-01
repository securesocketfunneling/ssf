#ifndef SSF_SERVICES_PROCESS_PROCESS_SERVER_H_
#define SSF_SERVICES_PROCESS_PROCESS_SERVER_H_

#include <string>

#include <boost/system/error_code.hpp>
#include <boost/asio/io_service.hpp>

#include <ssf/network/manager.h>
#include <ssf/network/base_session.h>

#include "services/base_service.h"
#include "services/service_id.h"

#include "common/boost/fiber/stream_fiber.hpp"
#include "common/boost/fiber/basic_fiber_demux.hpp"
#include "common/utils/to_underlying.h"

#include "core/factories/service_factory.h"

#include "services/admin/requests/create_service_request.h"
#include "services/process/config.h"

#if defined(BOOST_ASIO_WINDOWS)
#include "services/process/windows/session.h"
#else
#include "services/process/posix/session.h"
#endif  // defined(BOOST_ASIO_WINDOWS)

namespace ssf {
namespace services {
namespace process {

template <typename Demux>
class Server : public BaseService<Demux> {
 private:
#if defined(BOOST_ASIO_WINDOWS)
  using SessionImpl = windows::Session<Demux>;
#else
  using SessionImpl = posix::Session<Demux>;
#endif  // defined(BOOST_ASIO_WINDOWS)

  using LocalPortType = typename Demux::local_port_type;
  using ServerPtr = std::shared_ptr<Server>;
  using SessionManager = ItemManager<BaseSessionPtr>;

  using BaseServicePtr = std::shared_ptr<BaseService<Demux>>;
  using Parameters = typename ssf::BaseService<Demux>::Parameters;
  using Fiber = typename ssf::BaseService<Demux>::fiber;
  using FiberPtr = std::shared_ptr<Fiber>;
  using FiberAcceptor = typename ssf::BaseService<Demux>::fiber_acceptor;
  using FiberEndpoint = typename ssf::BaseService<Demux>::endpoint;

 public:
  // SSF service ID for identification in the service factory
  enum { kFactoryId = to_underlying(MicroserviceId::kProcessServer) };

 public:
  Server(const Server&) = delete;
  Server& operator=(const Server&) = delete;

  ~Server() { SSF_LOG("microservice", debug, "[shell] destroy"); }

 public:
  // Create a new instance of the service
  static ServerPtr Create(boost::asio::io_service& io_service,
                          Demux& fiber_demux, const Parameters& parameters,
                          const std::string& binary_path,
                          const std::string& binary_args) {
    if (!parameters.count("local_port") || binary_path.empty()) {
      return ServerPtr(nullptr);
    }

    uint32_t local_port;
    try {
      local_port = std::stoul(parameters.at("local_port"));
    } catch (const std::exception&) {
      SSF_LOG("microservice", error, "[shell]: cannot extract port parameter");
      return ServerPtr(nullptr);
    }

    return ServerPtr(new Server(io_service, fiber_demux, local_port,
                                binary_path, binary_args));
  }

  // Function used to register the micro service to the given factory
  static void RegisterToServiceFactory(
      std::shared_ptr<ServiceFactory<Demux>> p_factory, const Config& config) {
    if (!config.enabled()) {
      // service factory is not enabled
      return;
    }

    auto bin_path = config.path();
    auto bin_args = config.args();
    auto creator = [bin_path, bin_args](boost::asio::io_service& io_service,
                                        Demux& fiber_demux,
                                        const Parameters& parameters) {
      return Server::Create(io_service, fiber_demux, parameters, bin_path,
                            bin_args);
    };
    p_factory->RegisterServiceCreator(kFactoryId, creator);
  }

  // Function used to create service request
  static ssf::services::admin::CreateServiceRequest<Demux> GetCreateRequest(
      LocalPortType local_port) {
    ssf::services::admin::CreateServiceRequest<Demux> create(kFactoryId);
    create.add_parameter("local_port", std::to_string(local_port));

    return create;
  }

 public:
  // Start the service instance
  void start(boost::system::error_code& ec) override;

  // Stop the service instance
  void stop(boost::system::error_code& ec) override;

  // Return the type of the service
  uint32_t service_type_id() override;

 public:
  void StopSession(BaseSessionPtr session, boost::system::error_code& ec);

 private:
  Server(boost::asio::io_service& io_service, Demux& fiber_demux,
         const LocalPortType& port, const std::string& binary_path,
         const std::string& binary_args);

 private:
  void AsyncAcceptFiber();
  void FiberAcceptHandler(FiberPtr new_connection,
                          const boost::system::error_code& e);
  void HandleStop();

  ServerPtr SelfFromThis() {
    return std::static_pointer_cast<Server>(this->shared_from_this());
  }

  bool CheckBinaryPath();

 private:
  FiberAcceptor fiber_acceptor_;

 private:
  SessionManager session_manager_;
  boost::system::error_code init_ec_;
  LocalPortType local_port_;
  std::string binary_path_;
  std::string binary_args_;
};

}  // process
}  // services
}  // ssf

#include "services/process/server.ipp"

#endif  // SSF_SERVICES_PROCESS_PROCESS_SERVER_H_

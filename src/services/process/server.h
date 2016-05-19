#ifndef SSF_SERVICES_PROCESS_PROCESS_SERVER_H_
#define SSF_SERVICES_PROCESS_PROCESS_SERVER_H_

#include <string>
#include <map>

#include <boost/system/error_code.hpp>
#include <boost/asio.hpp>  // NOLINT

#include <ssf/network/manager.h>
#include <ssf/network/base_session.h>

#include "services/base_service.h"

#include "common/config/config.h"
#include "common/boost/fiber/stream_fiber.hpp"
#include "common/boost/fiber/basic_fiber_demux.hpp"

#include "core/factories/service_factory.h"

#include "services/admin/requests/create_service_request.h"

#if defined(BOOST_ASIO_WINDOWS)
#include "services/process/windows/session.h"
#else
#include "services/process/posix/session.h"
#endif

namespace ssf {
namespace services {
namespace process {

template <typename Demux>
class Server : public BaseService<Demux> {
 private:
#if defined(BOOST_ASIO_HAS_IOCP)
  using session_impl = windows::Session<Demux>;
#else
  using session_impl = class posix::Session<Demux>;
#endif

  using local_port_type = typename Demux::local_port_type;

  using ServerPtr = std::shared_ptr<Server>;
  using SessionManager = ItemManager<BaseSessionPtr>;
  using Parameters = typename ssf::BaseService<Demux>::Parameters;
  using demux = typename ssf::BaseService<Demux>::demux;
  using fiber = typename ssf::BaseService<Demux>::fiber;
  using fiber_acceptor = typename ssf::BaseService<Demux>::fiber_acceptor;
  using endpoint = typename ssf::BaseService<Demux>::endpoint;

 public:
  // SSF service ID for identification in the service factory
  enum { factory_id = 10 };

 public:
  Server(const Server&) = delete;
  Server& operator=(const Server&) = delete;

 public:
  // Create a new instance of the service
  static ServerPtr Create(boost::asio::io_service& io_service,
                          demux& fiber_demux, Parameters parameters,
                          const std::string& binary_path) {
    if (!parameters.count("local_port")) {
      return ServerPtr(nullptr);
    } else {
      return std::shared_ptr<Server>(
          new Server(io_service, fiber_demux,
                     std::stoul(parameters["local_port"]), binary_path));
    }
  }

  // Function used to register the micro service to the given factory
  static void RegisterToServiceFactory(
      std::shared_ptr<ServiceFactory<demux>> p_factory,
      const ssf::config::ProcessService& config) {
    p_factory->RegisterServiceCreator(
        factory_id,
        boost::bind(&Server::Create, _1, _2, _3, config.path()));
  }

  // Function used to generate create service request
  static ssf::services::admin::CreateServiceRequest<demux> GetCreateRequest(
      uint16_t local_port) {
    ssf::services::admin::CreateServiceRequest<demux> create(factory_id);
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

 private:
  Server(boost::asio::io_service& io_service, demux& fiber_demux,
         const local_port_type& port, const std::string& binary_path);

 private:
  void StartAccept();
  void HandleAccept(const boost::system::error_code& e);
  void HandleStop();

  std::shared_ptr<Server> SelfFromThis() {
    return std::static_pointer_cast<Server>(this->shared_from_this());
  }

  bool CheckBinaryPath();

 private:
  fiber_acceptor fiber_acceptor_;

 private:
  SessionManager session_manager_;
  fiber new_connection_;
  boost::system::error_code init_ec_;
  local_port_type local_port_;
  std::string binary_path_;
};

}  // process
}  // services
}  // ssf

#include "services/process/server.ipp"

#endif  // SSF_SERVICES_PROCESS_PROCESS_SERVER_H_

#ifndef SSF_SERVICES_PROCESS_PROCESS_SERVER_H_
#define SSF_SERVICES_PROCESS_PROCESS_SERVER_H_

#include <string>
#include <map>

#include <boost/system/error_code.hpp>
#include <boost/asio.hpp>  // NOLINT

#include <ssf/network/manager.h>
#include <ssf/network/base_session.h>

#include "services/base_service.h"

#include "common/boost/fiber/stream_fiber.hpp"
#include "common/boost/fiber/basic_fiber_demux.hpp"

#include "core/factories/service_factory.h"

#include "services/admin/requests/create_service_request.h"

namespace ssf {
namespace services {
namespace process {

template <typename Demux>
class Server : public BaseService<Demux> {
 private:
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
                          demux& fiber_demux, Parameters parameters) {
    if (!parameters.count("local_port")) {
      return ServerPtr(nullptr);
    } else {
      return std::shared_ptr<Server>(new Server(
          io_service, fiber_demux, std::stoul(parameters["local_port"])));
    }
  }

  // Function used to register the micro service to the given factory
  static void RegisterToServiceFactory(
      std::shared_ptr<ServiceFactory<demux>> p_factory) {
    p_factory->RegisterServiceCreator(factory_id, &Server::Create);
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
         const local_port_type& port);

 private:
  void StartAccept();
  void HandleAccept(const boost::system::error_code& e);
  void HandleStop();

  template <typename Handler, typename This>
  auto Then(Handler handler,
            This me) -> decltype(boost::bind(handler, me->SelfFromThis(), _1)) {
    return boost::bind(handler, me->SelfFromThis(), _1);
  }

  std::shared_ptr<Server> SelfFromThis() {
    return std::static_pointer_cast<Server>(this->shared_from_this());
  }

 private:
  fiber_acceptor fiber_acceptor_;

 private:
  SessionManager session_manager_;
  fiber new_connection_;
  boost::system::error_code init_ec_;
  local_port_type local_port_;
};

}  // process
}  // services
}  // ssf

#include "services/process/server.ipp"

#endif  // SSF_SERVICES_PROCESS_PROCESS_SERVER_H_
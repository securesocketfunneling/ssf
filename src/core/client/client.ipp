#ifndef SSF_CORE_CLIENT_CLIENT_IPP_
#define SSF_CORE_CLIENT_CLIENT_IPP_

#include <vector>

#include "services/admin/requests/create_service_request.h"
#include "services/admin/requests/stop_service_request.h"
#include "services/admin/requests/service_status.h"

#include "core/factories/service_factory.h"

#include "services/user_services/base_user_service.h"
#include "services/user_services/socks.h"
#include "services/user_services/remote_socks.h"
#include "services/user_services/port_forwarding.h"
#include "services/user_services/remote_port_forwarding.h"

namespace ssf {
template <typename P, template <class> class L,
          template <class, template <class> class> class N,
          template <class> class T>
SSFClient<P, L, N, T>::SSFClient(
    boost::asio::io_service& io_service, const std::string& remote_addr,
    const std::string& port, const ssf::Config& ssf_config,
    std::vector<BaseUserServicePtr> user_services,
    client_callback_type callback)
    : N<P, L>(io_service, ssf_config),
      T<typename P::socket_type>(
          boost::bind(&SSFClient<P, L, N, T>::do_ssf_start, this, _1, _2)),
      io_service_(io_service),
      fiber_demux_(io_service),
      user_services_(user_services),
      remote_addr_(remote_addr),
      remote_port_(port),
      callback_(std::move(callback)) {}

template <typename P,
          template <class> class L,
          template <class, template <class> class> class N,
          template <class> class T>
void SSFClient<P, L, N, T>::run(Parameters& parameters) {
  //parameters["remote_addr"] = remote_addr_;
  //parameters["remote_port"] = remote_port_;
  this->add_route(parameters, boost::bind(&SSFClient<P, L, N, T>::network_to_transport,
                                          this, _1, _2));
}

template <typename P,
          template <class> class L,
          template <class, template <class> class> class N,
          template <class> class T>
void SSFClient<P, L, N, T>::stop() {
  auto p_service_factory =
      ServiceFactoryManager<demux>::GetServiceFactory(&fiber_demux_);
  if (p_service_factory) {
    p_service_factory->Destroy();
  }

  fiber_demux_.close();
}

//-------------------------------------------------------------------------------
template <typename P,
          template <class> class L,
          template <class, template <class> class> class N,
          template <class> class T>
void SSFClient<P, L, N, T>::network_to_transport(p_socket_type p_socket,
                                                 vector_error_code_type v_ec) {
  if (print_ec_vector(v_ec)) {
    this->do_ssf_initiate(p_socket);
  } else {
    if (p_socket) {
      boost::system::error_code ec;
      p_socket->close(ec);
    }
    for (size_t i = 0; i < v_ec.size(); ++i) {
      if (v_ec[i]) {
        Notify(ssf::services::initialisation::NETWORK, nullptr, v_ec[i]);
        return;
      }
    }
  }
}
//------------------------------------------------------------------------------

template <typename P,
          template <class> class L,
          template <class, template <class> class> class N,
          template <class> class T>
bool SSFClient<P, L, N, T>::print_ec_vector(vector_error_code_type v_ec) {
  for (size_t i = 0; i < v_ec.size(); ++i) {
    BOOST_LOG_TRIVIAL(info) << "client: node " << i 
                            << " status: " << v_ec[i] 
                            << " message: " << v_ec[i].message();
    if (v_ec[i]) {
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
template <typename P,
          template <class> class L,
          template <class, template <class> class> class N,
          template <class> class T>
void SSFClient<P, L, N, T>::do_ssf_start(p_socket_type p_socket,
                                         const boost::system::error_code& ec) {
  Notify(ssf::services::initialisation::NETWORK, nullptr, ec);

  if (!ec) {
    BOOST_LOG_TRIVIAL(trace) << "client: SSF reply ok";
    boost::system::error_code ec2;
    this->do_fiberize_(p_socket, ec2);

    Notify(ssf::services::initialisation::TRANSPORT, nullptr, ec);
  } else {
    BOOST_LOG_TRIVIAL(error) << "client: SSF protocol error " << ec.message();
  }
}

template <typename P,
          template <class> class L,
          template <class, template <class> class> class N,
          template <class> class T>
void SSFClient<P, L, N, T>::do_fiberize_(p_socket_type p_socket,
                                         boost::system::error_code& ec) {
  // Register supported admin commands
  services::admin::CreateServiceRequest<demux>::RegisterToCommandFactory();
  services::admin::StopServiceRequest<demux>::RegisterToCommandFactory();
  services::admin::ServiceStatus<demux>::RegisterToCommandFactory();

  fiber_demux_.fiberize(std::move(*p_socket));

  // Make a new service manager
  auto p_service_manager = std::make_shared<ServiceManager<demux>>();

  // Make a new service factory
  auto p_service_factory = ServiceFactory<demux>::Create(
      io_service_, fiber_demux_, p_service_manager);

  // Register supported micro services
  services::socks::SocksServer<demux>::RegisterToServiceFactory(
      p_service_factory);
  services::fibers_to_sockets::RemoteForwarderService<
      demux>::RegisterToServiceFactory(p_service_factory);
  services::sockets_to_fibers::LocalForwarderService<
      demux>::RegisterToServiceFactory(p_service_factory);
  services::fibers_to_datagrams::FibersToDatagrams<
      demux>::RegisterToServiceFactory(p_service_factory);
  services::datagrams_to_fibers::DatagramsToFibers<
      demux>::RegisterToServiceFactory(p_service_factory);

  // Start the admin micro service
  std::map<std::string, std::string> empty_map;

  auto p_admin_service = services::admin::Admin<demux>::Create(
      io_service_, fiber_demux_, empty_map);
  p_admin_service->set_client(user_services_, callback_);
  p_service_manager->start(p_admin_service, ec);
}
//------------------------------------------------------------------------------

}  // ssf

#endif  // SSF_CORE_CLIENT_CLIENT_IPP_

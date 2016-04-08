#ifndef SSF_CORE_SERVER_SERVER_IPP_
#define SSF_CORE_SERVER_SERVER_IPP_

#include "services/admin/requests/create_service_request.h"
#include "services/admin/requests/stop_service_request.h"
#include "services/admin/requests/service_status.h"

#include "core/factories/service_factory.h"

namespace ssf {
template <typename P, template <class> class L,
          template <class, template <class> class> class N,
          template <class> class T>
SSFServer<P, L, N, T>::SSFServer(boost::asio::io_service& io_service,
                                 const ssf::Config& ssf_config,
                                 uint16_t local_port)
    : N<P, L>(io_service, ssf_config),
      T<typename P::socket_type>(
          boost::bind(&SSFServer<P, L, N, T>::DoSSFStart, this, _1, _2)),
      io_service_(io_service),
      local_port_(local_port) {}

/// Start accepting connections
template <typename P, template <class> class L,
          template <class, template <class> class> class N,
          template <class> class T>
void SSFServer<P, L, N, T>::Run() {
  // network policy
  this->AcceptNewRoutes(
      local_port_,
      boost::bind(&SSFServer<P, L, N, T>::NetworkToTransport, this, _1, _2));
}

/// Stop accepting connections and end all on going connections
template <typename P, template <class> class L,
          template <class, template <class> class> class N,
          template <class> class T>
void SSFServer<P, L, N, T>::Stop() {
  // network policy
  this->StopAcceptingRoutes();

  this->RemoveAllDemuxes();
}

//-------------------------------------------------------------------------------
template <typename P, template <class> class L,
          template <class, template <class> class> class N,
          template <class> class T>
void SSFServer<P, L, N, T>::NetworkToTransport(p_socket_type p_socket,
                                               vector_error_code_type v_ec) {
  if (!v_ec[0]) {
    this->DoSSFInitiateReceive(p_socket);
  } else {
    BOOST_LOG_TRIVIAL(error) << "server: network error: " << v_ec[0].message();
  }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
template <typename P, template <class> class L,
          template <class, template <class> class> class N,
          template <class> class T>
void SSFServer<P, L, N, T>::DoSSFStart(p_socket_type p_socket,
                                       const boost::system::error_code& ec) {
  if (!ec) {
    BOOST_LOG_TRIVIAL(trace) << "server: SSF reply ok";
    boost::system::error_code ec2;
    this->DoFiberize(p_socket, ec2);
  } else {
    BOOST_LOG_TRIVIAL(error) << "server: SSF protocol error " << ec.message();
  }
}

template <typename P, template <class> class L,
          template <class, template <class> class> class N,
          template <class> class T>
void SSFServer<P, L, N, T>::DoFiberize(p_socket_type p_socket,
                                       boost::system::error_code& ec) {
  // Register supported admin commands
  services::admin::CreateServiceRequest<demux>::RegisterToCommandFactory();
  services::admin::StopServiceRequest<demux>::RegisterToCommandFactory();
  services::admin::ServiceStatus<demux>::RegisterToCommandFactory();

  // Make a new fiber demux and fiberize
  auto p_fiber_demux = std::make_shared<demux>(io_service_);
  auto close_demux_handler =
      [this, p_fiber_demux]() { this->RemoveDemux(p_fiber_demux); };
  p_fiber_demux->fiberize(std::move(*p_socket), close_demux_handler);

  // Make a new service manager
  auto p_service_manager = std::make_shared<ServiceManager<demux>>();

  // Save the demux, the socket and the service manager
  this->AddDemux(p_fiber_demux, p_service_manager);

  // Make a new service factory
  auto p_service_factory = ServiceFactory<demux>::Create(
      io_service_, *p_fiber_demux, p_service_manager);

  // Register supported micro services
  services::socks::SocksServer<demux>::RegisterToServiceFactory(
      p_service_factory);
  services::fibers_to_sockets::FibersToSockets<demux>::RegisterToServiceFactory(
      p_service_factory);
  services::sockets_to_fibers::SocketsToFibers<demux>::RegisterToServiceFactory(
      p_service_factory);
  services::fibers_to_datagrams::FibersToDatagrams<
      demux>::RegisterToServiceFactory(p_service_factory);
  services::datagrams_to_fibers::DatagramsToFibers<
      demux>::RegisterToServiceFactory(p_service_factory);
  services::copy_file::file_to_fiber::FileToFiber<
      demux>::RegisterToServiceFactory(p_service_factory);
  services::copy_file::fiber_to_file::FiberToFile<
      demux>::RegisterToServiceFactory(p_service_factory);

  // Start the admin micro service
  std::map<std::string, std::string> empty_map;

  auto p_admin_service = services::admin::Admin<demux>::Create(
      io_service_, *p_fiber_demux, empty_map);
  p_admin_service->set_server();
  p_service_manager->start(p_admin_service, ec);
}
//------------------------------------------------------------------------------

template <typename P, template <class> class L,
          template <class, template <class> class> class N,
          template <class> class T>
void SSFServer<P, L, N, T>::AddDemux(p_demux p_fiber_demux,
                                     p_ServiceManager p_service_manager) {
  boost::recursive_mutex::scoped_lock lock(storage_mutex_);
  BOOST_LOG_TRIVIAL(trace) << "server: adding a new demux";

  p_fiber_demuxes_.insert(p_fiber_demux);
  p_service_managers_[p_fiber_demux] = p_service_manager;
}

template <typename P, template <class> class L,
          template <class, template <class> class> class N,
          template <class> class T>
void SSFServer<P, L, N, T>::RemoveDemux(p_demux p_fiber_demux) {
  boost::recursive_mutex::scoped_lock lock(storage_mutex_);
  BOOST_LOG_TRIVIAL(trace) << "server: removing a demux";

  p_fiber_demux->close();
  p_fiber_demuxes_.erase(p_fiber_demux);

  if (p_service_managers_.count(p_fiber_demux)) {
    auto p_service_manager = p_service_managers_[p_fiber_demux];
    p_service_manager->stop_all();
    p_service_managers_.erase(p_fiber_demux);
  }

  auto p_service_factory =
      ServiceFactoryManager<demux>::GetServiceFactory(p_fiber_demux.get());
  if (p_service_factory) {
    p_service_factory->Destroy();
  }
}

template <typename P, template <class> class L,
          template <class, template <class> class> class N,
          template <class> class T>
void SSFServer<P, L, N, T>::RemoveAllDemuxes() {
  boost::recursive_mutex::scoped_lock lock(storage_mutex_);
  BOOST_LOG_TRIVIAL(trace) << "server: removing all demuxes";

  for (auto& p_fiber_demux : p_fiber_demuxes_) {
    if (p_service_managers_.count(p_fiber_demux)) {
      auto p_service_manager = p_service_managers_[p_fiber_demux];
      if (p_service_manager) {
        p_service_manager->stop_all();
      }
      p_fiber_demux->close();
      p_service_managers_.erase(p_fiber_demux);
    }

    auto p_service_factory =
        ServiceFactoryManager<demux>::GetServiceFactory(p_fiber_demux.get());
    if (p_service_factory) {
      p_service_factory->Destroy();
    }
  }

  p_fiber_demuxes_.clear();
}

}  // ssf

#endif  // SSF_CORE_SERVER_SERVER_H_

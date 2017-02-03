#ifndef SSF_CORE_SERVER_SERVER_IPP_
#define SSF_CORE_SERVER_SERVER_IPP_

#include <ssf/log/log.h>

#include "common/error/error.h"

#include "core/factories/service_factory.h"

#include "services/admin/admin.h"
#include "services/admin/requests/create_service_request.h"
#include "services/admin/requests/stop_service_request.h"
#include "services/admin/requests/service_status.h"

#include "services/base_service.h"
#include "services/copy_file/file_to_fiber/file_to_fiber.h"
#include "services/copy_file/fiber_to_file/fiber_to_file.h"
#include "services/datagrams_to_fibers/datagrams_to_fibers.h"
#include "services/fibers_to_sockets/fibers_to_sockets.h"
#include "services/fibers_to_datagrams/fibers_to_datagrams.h"
#include "services/process/server.h"
#include "services/sockets_to_fibers/sockets_to_fibers.h"
#include "services/socks/socks_server.h"

namespace ssf {

template <class N, template <class> class T>
SSFServer<N, T>::SSFServer(const ssf::config::Services& services_config,
                           bool relay_only)
    : T<typename N::socket>(
          boost::bind(&SSFServer<N, T>::DoSSFStart, this, _1, _2)),
      async_engine_(),
      network_acceptor_(async_engine_.get_io_service()),
      services_config_(services_config),
      relay_only_(relay_only) {}

template <class N, template <class> class T>
SSFServer<N, T>::~SSFServer() {
  Stop();
}

/// Start accepting connections
template <class N, template <class> class T>
void SSFServer<N, T>::Run(const NetworkQuery& query,
                          boost::system::error_code& ec) {
  if (async_engine_.IsStarted()) {
    ec.assign(::error::device_or_resource_busy, ::error::get_ssf_category());
    SSF_LOG(kLogError) << "server: already running";
    return;
  }

  if (relay_only_) {
    SSF_LOG(kLogWarning) << "server: relay only";
  }

  // resolve remote endpoint with query
  NetworkResolver resolver(async_engine_.get_io_service());
  auto endpoint_it = resolver.resolve(query, ec);

  if (ec) {
    SSF_LOG(kLogError) << "server: could not resolve network endpoint";
    return;
  }

  // set acceptor
  network_acceptor_.open();

  /*
  // TODO: reuse address ?
  network_acceptor_.set_option(boost::asio::socket_base::reuse_address(true),
                               ec);
  */

  network_acceptor_.bind(*endpoint_it, ec);
  if (ec) {
    SSF_LOG(kLogError) << "server: could not bind acceptor to network endpoint";
    return;
  }

  network_acceptor_.listen(100, ec);
  if (ec) {
    SSF_LOG(kLogError) << "server: could not listen for new connections";
    return;
  }

  async_engine_.Start();

  // start accepting connection
  AsyncAcceptConnection();
}

/// Stop accepting connections and end all on going connections
template <class N, template <class> class T>
void SSFServer<N, T>::Stop() {
  if (!async_engine_.IsStarted()) {
    return;
  }

  SSF_LOG(kLogDebug) << "server: stop";

  RemoveAllDemuxes();

  // close acceptor
  boost::system::error_code close_ec;
  network_acceptor_.close(close_ec);

  async_engine_.Stop();
}

template <class N, template <class> class T>
boost::asio::io_service& SSFServer<N, T>::get_io_service() {
  return async_engine_.get_io_service();
}

template <class N, template <class> class T>
void SSFServer<N, T>::AsyncAcceptConnection() {
  if (network_acceptor_.is_open()) {
    NetworkSocketPtr p_socket =
        std::make_shared<NetworkSocket>(async_engine_.get_io_service());

    network_acceptor_.async_accept(
        *p_socket,
        boost::bind(&SSFServer::NetworkToTransport, this, _1, p_socket));
  }
}

template <class N, template <class> class T>
void SSFServer<N, T>::NetworkToTransport(const boost::system::error_code& ec,
                                         NetworkSocketPtr p_socket) {
  AsyncAcceptConnection();

  if (!ec) {
    if (!relay_only_) {
      this->DoSSFInitiateReceive(p_socket);
      return;
    }
  }

  // Close connection
  boost::system::error_code close_ec;
  p_socket->shutdown(boost::asio::socket_base::shutdown_both, close_ec);
  p_socket->close(close_ec);
  if (ec) {
    SSF_LOG(kLogError) << "server: network error: " << ec.message();
  } else if (relay_only_) {
    SSF_LOG(kLogWarning)
        << "server: direct connection attempt with relay-only option";
  }
}

template <class N, template <class> class T>
void SSFServer<N, T>::DoSSFStart(NetworkSocketPtr p_socket,
                                 const boost::system::error_code& ec) {
  if (!ec) {
    SSF_LOG(kLogTrace) << "server: SSF reply ok";
    boost::system::error_code ec2;
    DoFiberize(p_socket, ec2);
  } else {
    boost::system::error_code close_ec;
    p_socket->shutdown(boost::asio::socket_base::shutdown_both, close_ec);
    p_socket->close(close_ec);
    SSF_LOG(kLogError) << "server: SSF protocol error " << ec.message();
  }
}

template <class N, template <class> class T>
void SSFServer<N, T>::DoFiberize(NetworkSocketPtr p_socket,
                                 boost::system::error_code& ec) {
  boost::recursive_mutex::scoped_lock lock(storage_mutex_);

  // Register supported admin commands
  services::admin::CreateServiceRequest<demux>::RegisterToCommandFactory();
  services::admin::StopServiceRequest<demux>::RegisterToCommandFactory();
  services::admin::ServiceStatus<demux>::RegisterToCommandFactory();

  // Make a new fiber demux and fiberize
  auto p_fiber_demux = std::make_shared<demux>(async_engine_.get_io_service());
  auto close_demux_handler =
      [this, p_fiber_demux]() { RemoveDemux(p_fiber_demux); };
  p_fiber_demux->fiberize(std::move(*p_socket), close_demux_handler);

  // Make a new service manager
  auto p_service_manager = std::make_shared<ServiceManager<demux>>();

  // Save the demux, the socket and the service manager
  AddDemux(p_fiber_demux, p_service_manager);

  // Make a new service factory
  auto p_service_factory = ServiceFactory<demux>::Create(
      async_engine_.get_io_service(), *p_fiber_demux, p_service_manager);

  // Register supported microservices
  services::socks::SocksServer<demux>::RegisterToServiceFactory(
      p_service_factory, services_config_.socks());
  services::fibers_to_sockets::FibersToSockets<demux>::RegisterToServiceFactory(
      p_service_factory, services_config_.stream_forwarder());
  services::sockets_to_fibers::SocketsToFibers<demux>::RegisterToServiceFactory(
      p_service_factory, services_config_.stream_listener());
  services::fibers_to_datagrams::FibersToDatagrams<
      demux>::RegisterToServiceFactory(p_service_factory,
                                       services_config_.datagram_forwarder());
  services::datagrams_to_fibers::DatagramsToFibers<
      demux>::RegisterToServiceFactory(p_service_factory,
                                       services_config_.datagram_listener());
  services::copy_file::file_to_fiber::FileToFiber<
      demux>::RegisterToServiceFactory(p_service_factory,
                                       services_config_.file_copy());
  services::copy_file::fiber_to_file::FiberToFile<
      demux>::RegisterToServiceFactory(p_service_factory,
                                       services_config_.file_copy());
  services::process::Server<demux>::RegisterToServiceFactory(
      p_service_factory, services_config_.process());

  // Start the admin microservice
  std::map<std::string, std::string> empty_map;

  auto p_admin_service = services::admin::Admin<demux>::Create(
      async_engine_.get_io_service(), *p_fiber_demux, empty_map);
  p_admin_service->set_server();
  p_service_manager->start(p_admin_service, ec);
}

template <class N, template <class> class T>
void SSFServer<N, T>::AddDemux(DemuxPtr p_fiber_demux,
                               ServiceManagerPtr p_service_manager) {
  boost::recursive_mutex::scoped_lock lock(storage_mutex_);
  SSF_LOG(kLogTrace) << "server: adding a new demux";

  p_fiber_demuxes_.insert(p_fiber_demux);
  p_service_managers_[p_fiber_demux] = p_service_manager;
}

template <class N, template <class> class T>
void SSFServer<N, T>::RemoveDemux(DemuxPtr p_fiber_demux) {
  boost::recursive_mutex::scoped_lock lock(storage_mutex_);
  SSF_LOG(kLogTrace) << "server: removing a demux";

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

template <class N, template <class> class T>
void SSFServer<N, T>::RemoveAllDemuxes() {
  boost::recursive_mutex::scoped_lock lock(storage_mutex_);
  SSF_LOG(kLogTrace) << "server: removing all demuxes";

  for (auto& p_fiber_demux : p_fiber_demuxes_) {
    p_fiber_demux->close();

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

  p_fiber_demuxes_.clear();
}

}  // ssf

#endif  // SSF_CORE_SERVER_SERVER_H_

#ifndef SSF_CORE_SERVER_SERVER_IPP_
#define SSF_CORE_SERVER_SERVER_IPP_

#include "common/error/error.h"

#include "services/admin/requests/create_service_request.h"
#include "services/admin/requests/stop_service_request.h"
#include "services/admin/requests/service_status.h"

#include "core/factories/service_factory.h"

namespace ssf {

template <class N, template <class> class T>
SSFServer<N, T>::SSFServer(const ssf::Config& ssf_config, uint16_t local_port)
    : AsyncRunner(),
      T<typename N::socket>(
          boost::bind(&SSFServer<N, T>::DoSSFStart, this, _1, _2)),
      network_acceptor_(io_service_),
      local_port_(local_port) {}

template <class N, template <class> class T>
SSFServer<N, T>::~SSFServer() {}

/// Start accepting connections
template <class N, template <class> class T>
void SSFServer<N, T>::Run(const network_query_type& query,
                          boost::system::error_code& ec) {
  if (p_worker_.get() != nullptr) {
    ec.assign(::error::device_or_resource_busy, ::error::get_ssf_category());
    BOOST_LOG_TRIVIAL(error) << "Server already running";
    return;
  }

  p_worker_.reset(new boost::asio::io_service::work(io_service_));

  // resolve remote endpoint with query
  network_resolver_type resolver(io_service_);
  boost::system::error_code resolve_ec;
  auto endpoint_it = resolver.resolve(query, resolve_ec);

  if (resolve_ec) {
    BOOST_LOG_TRIVIAL(error) << "Could not resolve network endpoint";
    return;
  }

  // set acceptor
  boost::system::error_code acceptor_ec;
  network_acceptor_.open();
  network_acceptor_.set_option(boost::asio::socket_base::reuse_address(true),
                               acceptor_ec);
  network_acceptor_.bind(*endpoint_it, acceptor_ec);
  if (acceptor_ec) {
    BOOST_LOG_TRIVIAL(error) << "Could not bind acceptor to network endpoint";
    return;
  }

  network_acceptor_.listen(100, acceptor_ec);
  if (acceptor_ec) {
    BOOST_LOG_TRIVIAL(error) << "Could not listen for new connections";
    return;
  }

  StartAsyncEngine();

  // start accepting connection
  AsyncAcceptConnection();
}

/// Stop accepting connections and end all on going connections
template <class N, template <class> class T>
void SSFServer<N, T>::Stop() {
  RemoveAllDemuxes();

  // close acceptor
  boost::system::error_code close_ec;
  network_acceptor_.close(close_ec);

  p_worker_.reset(nullptr);

  StopAsyncEngine();
}

template <class N, template <class> class T>
void SSFServer<N, T>::AsyncAcceptConnection() {
  if (network_acceptor_.is_open()) {
    p_network_socket_type p_socket =
        std::make_shared<network_socket_type>(io_service_);

    network_acceptor_.async_accept(
        *p_socket,
        boost::bind(&SSFServer::NetworkToTransport, this, _1, p_socket));
  }
}

template <class N, template <class> class T>
void SSFServer<N, T>::NetworkToTransport(const boost::system::error_code& ec,
                                         p_network_socket_type p_socket) {
  if (!ec) {
    this->DoSSFInitiateReceive(p_socket);
    AsyncAcceptConnection();
  } else {
    boost::system::error_code close_ec;
    p_socket->shutdown(boost::asio::socket_base::shutdown_both, close_ec);
    p_socket->close(close_ec);
    BOOST_LOG_TRIVIAL(error) << "server: network error: " << ec.message();
  }
}

template <class N, template <class> class T>
void SSFServer<N, T>::DoSSFStart(p_network_socket_type p_socket,
                                 const boost::system::error_code& ec) {
  if (!ec) {
    BOOST_LOG_TRIVIAL(trace) << "server: SSF reply ok";
    boost::system::error_code ec2;
    DoFiberize(p_socket, ec2);
  } else {
    boost::system::error_code close_ec;
    p_socket->shutdown(boost::asio::socket_base::shutdown_both, close_ec);
    p_socket->close(close_ec);
    BOOST_LOG_TRIVIAL(error) << "server: SSF protocol error " << ec.message();
  }
}

template <class N, template <class> class T>
void SSFServer<N, T>::DoFiberize(p_network_socket_type p_socket,
                                 boost::system::error_code& ec) {
  boost::recursive_mutex::scoped_lock lock(storage_mutex_);

  // Register supported admin commands
  services::admin::CreateServiceRequest<demux>::RegisterToCommandFactory();
  services::admin::StopServiceRequest<demux>::RegisterToCommandFactory();
  services::admin::ServiceStatus<demux>::RegisterToCommandFactory();

  // Make a new fiber demux and fiberize
  auto p_fiber_demux = std::make_shared<demux>(io_service_);
  auto close_demux_handler =
      [this, p_fiber_demux]() { RemoveDemux(p_fiber_demux); };
  p_fiber_demux->fiberize(std::move(*p_socket), close_demux_handler);

  // Make a new service manager
  auto p_service_manager = std::make_shared<ServiceManager<demux>>();

  // Save the demux, the socket and the service manager
  AddDemux(p_fiber_demux, p_service_manager);

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

template <class N, template <class> class T>
void SSFServer<N, T>::AddDemux(p_demux p_fiber_demux,
                               p_ServiceManager p_service_manager) {
  boost::recursive_mutex::scoped_lock lock(storage_mutex_);
  BOOST_LOG_TRIVIAL(trace) << "server: adding a new demux";

  p_fiber_demuxes_.insert(p_fiber_demux);
  p_service_managers_[p_fiber_demux] = p_service_manager;
}

template <class N, template <class> class T>
void SSFServer<N, T>::RemoveDemux(p_demux p_fiber_demux) {
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

template <class N, template <class> class T>
void SSFServer<N, T>::RemoveAllDemuxes() {
  boost::recursive_mutex::scoped_lock lock(storage_mutex_);
  BOOST_LOG_TRIVIAL(trace) << "server: removing all demuxes";

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

#ifndef SSF_CORE_CLIENT_CLIENT_IPP_
#define SSF_CORE_CLIENT_CLIENT_IPP_

#include "common/error/error.h"

#include "services/admin/requests/create_service_request.h"
#include "services/admin/requests/stop_service_request.h"
#include "services/admin/requests/service_status.h"

#include "core/factories/service_factory.h"

#include "services/user_services/base_user_service.h"
#include "services/user_services/socks.h"
#include "services/user_services/remote_socks.h"
#include "services/user_services/port_forwarding.h"
#include "services/user_services/remote_port_forwarding.h"
#include "services/user_services/copy_file_service.h"

namespace ssf {

template <class N, template <class> class T>
SSFClient<N, T>::SSFClient(boost::asio::io_service& io_service,
                           std::vector<BaseUserServicePtr> user_services,
                           client_callback_type callback)
    : T<typename N::socket>(
          boost::bind(&SSFClient<N, T>::DoSSFStart, this, _1, _2)),
      io_service_(io_service),
      p_worker_(nullptr),
      fiber_demux_(io_service),
      user_services_(user_services),
      callback_(std::move(callback)) {}

template <class N, template <class> class T>
void SSFClient<N, T>::Run(const network_query_type& query,
                          boost::system::error_code& ec) {
  if (p_worker_.get() != nullptr) {
    ec.assign(::error::device_or_resource_busy, ::error::get_ssf_category());
    BOOST_LOG_TRIVIAL(error) << "Client already running";
    return;
  }

  p_worker_.reset(new boost::asio::io_service::work(io_service_));
  // Create network socket
  p_network_socket_type p_socket =
      std::make_shared<network_socket_type>(io_service_);
  
  // resolve remote endpoint with query
  network_resolver_type resolver(io_service_);
  auto endpoint_it = resolver.resolve(query, ec);

  if (ec) {
    Notify(ssf::services::initialisation::NETWORK, nullptr, ec);
    BOOST_LOG_TRIVIAL(error) << "Could not resolve network endpoint";
    return;
  }

  // async connect client to given endpoint
  p_socket->async_connect(
      *endpoint_it,
      boost::bind(&SSFClient<N, T>::NetworkToTransport, this, _1, p_socket));
}

template <class N, template <class> class T>
void SSFClient<N, T>::Stop() {
  fiber_demux_.close();
  p_worker_.reset(nullptr);
}

//-------------------------------------------------------------------------------
template <class N, template <class> class T>
void SSFClient<N, T>::NetworkToTransport(const boost::system::error_code& ec,
                                         p_network_socket_type p_socket) {
  if (!ec) {
    this->DoSSFInitiate(p_socket);
    return;
  }

  BOOST_LOG_TRIVIAL(error) << "Error when connecting to server "
                           << ec.message();

  if (p_socket) {
    boost::system::error_code close_ec;
    p_socket->close(close_ec);
  }

  Notify(ssf::services::initialisation::NETWORK, nullptr, ec);
}

//------------------------------------------------------------------------------
template <class N, template <class> class T>
void SSFClient<N, T>::DoSSFStart(p_network_socket_type p_socket,
                                 const boost::system::error_code& ec) {
  Notify(ssf::services::initialisation::NETWORK, nullptr, ec);

  if (!ec) {
    BOOST_LOG_TRIVIAL(trace) << "client: SSF reply ok";
    boost::system::error_code ec2;
    this->DoFiberize(p_socket, ec2);

    Notify(ssf::services::initialisation::TRANSPORT, nullptr, ec);
  } else {
    BOOST_LOG_TRIVIAL(error) << "client: SSF protocol error " << ec.message();
  }
}

template <class N, template <class> class T>
void SSFClient<N, T>::DoFiberize(p_network_socket_type p_socket,
                                 boost::system::error_code& ec) {
  // Register supported admin commands
  services::admin::CreateServiceRequest<demux>::RegisterToCommandFactory();
  services::admin::StopServiceRequest<demux>::RegisterToCommandFactory();
  services::admin::ServiceStatus<demux>::RegisterToCommandFactory();

  auto close_demux_handler = [this]() { this->OnDemuxClose(); };
  fiber_demux_.fiberize(std::move(*p_socket), close_demux_handler);

  // Make a new service manager
  auto p_service_manager = std::make_shared<ServiceManager<demux>>();

  // Make a new service factory
  auto p_service_factory = ServiceFactory<demux>::Create(
      io_service_, fiber_demux_, p_service_manager);

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
  services::copy_file::file_enquirer::FileEnquirer<
      demux>::RegisterToServiceFactory(p_service_factory);

  // Start the admin micro service
  std::map<std::string, std::string> empty_map;

  auto p_admin_service = services::admin::Admin<demux>::Create(
      io_service_, fiber_demux_, empty_map);
  p_admin_service->set_client(user_services_, callback_);
  p_service_manager->start(p_admin_service, ec);
}
//------------------------------------------------------------------------------

template <class N, template <class> class T>
void SSFClient<N, T>::OnDemuxClose() {
  auto p_service_factory =
      ServiceFactoryManager<demux>::GetServiceFactory(&fiber_demux_);
  if (p_service_factory) {
    p_service_factory->Destroy();
  }
  Notify(ssf::services::initialisation::CLOSE, nullptr,
         boost::system::error_code());
}

}  // ssf

#endif  // SSF_CORE_CLIENT_CLIENT_IPP_

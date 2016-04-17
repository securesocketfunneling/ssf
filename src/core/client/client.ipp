#ifndef SSF_CORE_CLIENT_CLIENT_IPP_
#define SSF_CORE_CLIENT_CLIENT_IPP_

#include "common/error/error.h"

#include "core/factories/service_factory.h"

#include "services/admin/admin.h"
#include "services/admin/requests/create_service_request.h"
#include "services/admin/requests/stop_service_request.h"
#include "services/admin/requests/service_status.h"

#include "services/copy_file/fiber_to_file/fiber_to_file.h"
#include "services/copy_file/file_enquirer/file_enquirer.h"
#include "services/copy_file/file_to_fiber/file_to_fiber.h"
#include "services/datagrams_to_fibers/datagrams_to_fibers.h"
#include "services/fibers_to_datagrams/fibers_to_datagrams.h"
#include "services/fibers_to_datagrams/fibers_to_datagrams.h"
#include "services/fibers_to_sockets/fibers_to_sockets.h"
#include "services/sockets_to_fibers/sockets_to_fibers.h"
#include "services/socks/socks_server.h"

namespace ssf {

template <class N, template <class> class T>
SSFClient<N, T>::SSFClient(std::vector<BaseUserServicePtr> user_services,
                           ClientCallback callback)
    : T<typename N::socket>(
          boost::bind(&SSFClient<N, T>::DoSSFStart, this, _1, _2)),
      async_engine_(),
      fiber_demux_(async_engine_.get_io_service()),
      user_services_(user_services),
      callback_(std::move(callback)) {}

template <class N, template <class> class T>
SSFClient<N, T>::~SSFClient() {
  Stop();
}

template <class N, template <class> class T>
void SSFClient<N, T>::Run(const NetworkQuery& query,
                          boost::system::error_code& ec) {
  if (async_engine_.IsStarted()) {
    ec.assign(::error::device_or_resource_busy, ::error::get_ssf_category());
    BOOST_LOG_TRIVIAL(error) << "Client already running";
    return;
  }

  // Create network socket
  NetworkSocketPtr p_socket =
      std::make_shared<NetworkSocket>(async_engine_.get_io_service());

  // resolve remote endpoint with query
  NetworkResolver resolver(async_engine_.get_io_service());
  auto endpoint_it = resolver.resolve(query, ec);

  async_engine_.Start();

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

  async_engine_.Stop();
}

template <class N, template <class> class T>
void SSFClient<N, T>::NetworkToTransport(const boost::system::error_code& ec,
                                         NetworkSocketPtr p_socket) {
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

template <class N, template <class> class T>
void SSFClient<N, T>::DoSSFStart(NetworkSocketPtr p_socket,
                                 const boost::system::error_code& ec) {
  Notify(ssf::services::initialisation::NETWORK, nullptr, ec);

  if (!ec) {
    BOOST_LOG_TRIVIAL(trace) << "client: SSF reply ok";
    boost::system::error_code ec2;
    DoFiberize(p_socket, ec2);

    Notify(ssf::services::initialisation::TRANSPORT, nullptr, ec);
  } else {
    BOOST_LOG_TRIVIAL(error) << "client: SSF protocol error " << ec.message();
  }
}

template <class N, template <class> class T>
void SSFClient<N, T>::DoFiberize(NetworkSocketPtr p_socket,
                                 boost::system::error_code& ec) {
  // Register supported admin commands
  services::admin::CreateServiceRequest<Demux>::RegisterToCommandFactory();
  services::admin::StopServiceRequest<Demux>::RegisterToCommandFactory();
  services::admin::ServiceStatus<Demux>::RegisterToCommandFactory();

  auto close_demux_handler = [this]() { OnDemuxClose(); };
  fiber_demux_.fiberize(std::move(*p_socket), close_demux_handler);

  // Make a new service manager
  auto p_service_manager = std::make_shared<ServiceManager<Demux>>();

  // Make a new service factory
  auto p_service_factory = ServiceFactory<Demux>::Create(
      async_engine_.get_io_service(), fiber_demux_, p_service_manager);

  // Register supported micro services
  services::socks::SocksServer<Demux>::RegisterToServiceFactory(
      p_service_factory);
  services::fibers_to_sockets::FibersToSockets<Demux>::RegisterToServiceFactory(
      p_service_factory);
  services::sockets_to_fibers::SocketsToFibers<Demux>::RegisterToServiceFactory(
      p_service_factory);
  services::fibers_to_datagrams::FibersToDatagrams<
      Demux>::RegisterToServiceFactory(p_service_factory);
  services::datagrams_to_fibers::DatagramsToFibers<
      Demux>::RegisterToServiceFactory(p_service_factory);
  services::copy_file::file_to_fiber::FileToFiber<
      Demux>::RegisterToServiceFactory(p_service_factory);
  services::copy_file::fiber_to_file::FiberToFile<
      Demux>::RegisterToServiceFactory(p_service_factory);
  services::copy_file::file_enquirer::FileEnquirer<
      Demux>::RegisterToServiceFactory(p_service_factory);

  // Start the admin micro service
  std::map<std::string, std::string> empty_map;

  auto p_admin_service = services::admin::Admin<Demux>::Create(
      async_engine_.get_io_service(), fiber_demux_, empty_map);
  p_admin_service->set_client(user_services_, callback_);
  p_service_manager->start(p_admin_service, ec);
}

template <class N, template <class> class T>
void SSFClient<N, T>::OnDemuxClose() {
  auto p_service_factory =
      ServiceFactoryManager<Demux>::GetServiceFactory(&fiber_demux_);
  if (p_service_factory) {
    p_service_factory->Destroy();
  }
  Notify(ssf::services::initialisation::CLOSE, nullptr,
         boost::system::error_code());
}

}  // ssf

#endif  // SSF_CORE_CLIENT_CLIENT_IPP_

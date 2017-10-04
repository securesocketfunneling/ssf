#ifndef SSF_CORE_CLIENT_SESSION_IPP_
#define SSF_CORE_CLIENT_SESSION_IPP_

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
#include "services/fibers_to_sockets/fibers_to_sockets.h"
#include "services/process/server.h"
#include "services/sockets_to_fibers/sockets_to_fibers.h"
#include "services/socks/socks_server.h"

#include "ssf/log/log.h"

namespace ssf {

template <class N, template <class> class T>
std::shared_ptr<Session<N, T>> Session<N, T>::Create(
    boost::asio::io_service& io_service,
    std::vector<BaseUserServicePtr> user_services,
    const ssf::config::Services& services_config, OnStatusCb on_status,
    OnUserServiceStatusCb on_user_service_status,
    boost::system::error_code& ec) {
  std::shared_ptr<Session<N, T>> p_session(
      new Session(io_service, user_services, services_config, on_status,
                  on_user_service_status));

  return p_session;
}

template <class N, template <class> class T>
Session<N, T>::Session(boost::asio::io_service& io_service,
                       const std::vector<BaseUserServicePtr>& user_services,
                       const ssf::config::Services& services_config,
                       OnStatusCb on_status,
                       OnUserServiceStatusCb on_user_service_status)
    : T<typename N::socket>(),
      io_service_(io_service),
      user_services_(user_services),
      services_config_(services_config),
      fiber_demux_(io_service),
      stopped_(false),
      status_(Status::kInitialized),
      on_status_(on_status),
      on_user_service_status_(on_user_service_status) {}

template <class N, template <class> class T>
Session<N, T>::~Session() {
  SSF_LOG(kLogDebug) << "client session: destroy";
}

template <class N, template <class> class T>
void Session<N, T>::Start(const NetworkQuery& query,
                          boost::system::error_code& ec) {
  auto self = shared_from_this();

  // create network socket
  p_socket_ = std::make_shared<NetworkSocket>(io_service_);

  // create new service manager
  p_service_manager_ = std::make_shared<ServiceManager<Demux>>();

  // resolve remote endpoint with query
  NetworkResolver resolver(io_service_);
  auto endpoint_it = resolver.resolve(query, ec);
  if (ec) {
    SSF_LOG(kLogError) << "client session: could not resolve network endpoint";
    UpdateStatus(Status::kEndpointNotResolvable);
    return;
  }

  // async connect to given endpoint
  p_socket_->async_connect(
      *endpoint_it, std::bind(&Session<N, T>::NetworkToTransport,
                              shared_from_this(), std::placeholders::_1));
}

template <class N, template <class> class T>
void Session<N, T>::Stop(boost::system::error_code& ec) {
  if (stopped_) {
    return;
  }

  stopped_ = true;

  SSF_LOG(kLogDebug) << "client session: stop";

  fiber_demux_.close();
  if (p_service_manager_) {
    p_service_manager_->stop_all();
  }
  // p_service_manager_.reset();

  if (p_socket_) {
    boost::system::error_code close_ec;
    p_socket_->shutdown(boost::asio::socket_base::shutdown_both, close_ec);
    p_socket_->close(close_ec);
  }

  // p_socket_.reset();

  auto self = shared_from_this();
  if (status_ == Status::kConnected || status_ == Status::kRunning) {
    io_service_.post(std::bind(&Session<N, T>::UpdateStatus, shared_from_this(),
                               Status::kDisconnected));
  }
}

template <class N, template <class> class T>
void Session<N, T>::NetworkToTransport(const boost::system::error_code& ec) {
  if (ec) {
    SSF_LOG(kLogError) << "client session: server connection error: "
                       << ec.message();
    UpdateStatus(Status::kServerUnreachable);
    return;
  }

  UpdateStatus(Status::kConnected);
  this->DoSSFInitiate(p_socket_,
                      std::bind(&Session::DoSSFStart, shared_from_this(),
                                std::placeholders::_1, std::placeholders::_2));
}

template <class N, template <class> class T>
void Session<N, T>::DoSSFStart(NetworkSocketPtr p_socket,
                               const boost::system::error_code& ec) {
  boost::system::error_code stop_ec;

  if (ec) {
    SSF_LOG(kLogError) << "client session: SSF protocol error: "
                       << ec.message();
    UpdateStatus(Status::kServerNotSupported);
    return;
  }

  SSF_LOG(kLogTrace) << "client session: SSF reply ok";
  boost::system::error_code fiberize_ec;
  DoFiberize(p_socket, fiberize_ec);
  if (fiberize_ec) {
    UpdateStatus(Status::kServerNotSupported);
    return;
  }
}

template <class N, template <class> class T>
void Session<N, T>::DoFiberize(NetworkSocketPtr p_socket,
                               boost::system::error_code& ec) {
  // Register supported admin commands
  services::admin::CreateServiceRequest<Demux>::RegisterToCommandFactory();
  services::admin::StopServiceRequest<Demux>::RegisterToCommandFactory();
  services::admin::ServiceStatus<Demux>::RegisterToCommandFactory();

  auto self = shared_from_this();
  auto close_demux_handler = [this, self]() { OnDemuxClose(); };
  fiber_demux_.fiberize(std::move(*p_socket), close_demux_handler);

  p_socket_.reset();

  // Make a new service factory
  auto p_service_factory = ServiceFactory<Demux>::Create(
      io_service_, fiber_demux_, p_service_manager_);

  // Register supported micro services
  services::socks::SocksServer<Demux>::RegisterToServiceFactory(
      p_service_factory, services_config_.socks());
  services::fibers_to_sockets::FibersToSockets<Demux>::RegisterToServiceFactory(
      p_service_factory, services_config_.stream_forwarder());
  services::sockets_to_fibers::SocketsToFibers<Demux>::RegisterToServiceFactory(
      p_service_factory, services_config_.stream_listener());
  services::fibers_to_datagrams::FibersToDatagrams<
      Demux>::RegisterToServiceFactory(p_service_factory,
                                       services_config_.datagram_forwarder());
  services::datagrams_to_fibers::DatagramsToFibers<
      Demux>::RegisterToServiceFactory(p_service_factory,
                                       services_config_.datagram_listener());
  services::copy_file::file_to_fiber::FileToFiber<
      Demux>::RegisterToServiceFactory(p_service_factory,
                                       services_config_.file_copy());
  services::copy_file::fiber_to_file::FiberToFile<
      Demux>::RegisterToServiceFactory(p_service_factory,
                                       services_config_.file_copy());
  services::copy_file::file_enquirer::FileEnquirer<
      Demux>::RegisterToServiceFactory(p_service_factory,
                                       services_config_.file_copy());
  services::process::Server<Demux>::RegisterToServiceFactory(
      p_service_factory, services_config_.process());

  // Start the admin micro service

  auto on_user_service_status = [this, self](
      BaseUserServicePtr p_service, const boost::system::error_code& ec) {
    on_user_service_status_(p_service, ec);
  };

  auto p_admin_service =
      services::admin::Admin<Demux>::Create(io_service_, fiber_demux_, {});
  p_admin_service->SetClient(user_services_, on_user_service_status);
  p_service_manager_->start(p_admin_service, ec);

  UpdateStatus(Status::kRunning);
}

template <class N, template <class> class T>
void Session<N, T>::OnDemuxClose() {
  auto p_service_factory =
      ServiceFactoryManager<Demux>::GetServiceFactory(&fiber_demux_);
  if (p_service_factory) {
    p_service_factory->Destroy();
  }

  UpdateStatus(Status::kDisconnected);
}

template <class N, template <class> class T>
void Session<N, T>::UpdateStatus(Status status) {
  if (status_ == status) {
    return;
  }

  status_ = status;
  on_status_(status);
}

}  // ssf

#endif  // SSF_CORE_CLIENT_SESSION_IPP_

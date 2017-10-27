#ifndef SSF_CORE_CLIENT_SESSION_IPP_
#define SSF_CORE_CLIENT_SESSION_IPP_

#include "common/error/error.h"

#include "core/factories/service_factory.h"

#include "services/admin/admin.h"
#include "services/admin/requests/create_service_request.h"
#include "services/admin/requests/service_status.h"
#include "services/admin/requests/stop_service_request.h"

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
  SSF_LOG(kLogDebug) << "[client][session] destroy";
}

template <class N, template <class> class T>
void Session<N, T>::Start(const NetworkQuery& query,
                          boost::system::error_code& ec) {
  // create network socket
  p_socket_ = std::make_shared<NetworkSocket>(io_service_);

  // create new service manager
  p_service_manager_ = std::make_shared<ServiceManager<Demux>>();

  // resolve remote endpoint with query
  NetworkResolver resolver(io_service_);
  auto endpoint_it = resolver.resolve(query, ec);
  if (ec) {
    SSF_LOG(kLogError)
        << "[client][session] could not resolve network endpoint";
    UpdateStatus(Status::kEndpointNotResolvable);
    return;
  }

  // async connect to given endpoint
  auto self = this->shared_from_this();
  auto on_connect = [this, self](const boost::system::error_code& ec) {
    NetworkToTransport(ec);
  };
  p_socket_->async_connect(*endpoint_it, on_connect);
}

template <class N, template <class> class T>
void Session<N, T>::Stop(boost::system::error_code& ec) {
  if (stopped_) {
    return;
  }

  stopped_ = true;

  SSF_LOG(kLogDebug) << "[client][session] stop";

  fiber_demux_.close();
  if (p_service_manager_) {
    p_service_manager_->stop_all();
  }

  if (p_socket_) {
    boost::system::error_code close_ec;
    p_socket_->shutdown(boost::asio::socket_base::shutdown_both, close_ec);
    p_socket_->close(close_ec);
  }

  auto self = this->shared_from_this();
  if (status_ == Status::kConnected || status_ == Status::kRunning) {
    auto update_status = [this, self]() {
      UpdateStatus(Status::kDisconnected);
    };
    io_service_.post(update_status);
  }
}

template <class N, template <class> class T>
void Session<N, T>::NetworkToTransport(const boost::system::error_code& ec) {
  if (ec) {
    SSF_LOG(kLogDebug) << "[client][session] server connection error: "
                       << ec.message();
    UpdateStatus(Status::kServerUnreachable);
    return;
  }

  UpdateStatus(Status::kConnected);
  auto self = this->shared_from_this();
  auto on_ssf_initiate = [this, self](NetworkSocket& socket,
                                      const boost::system::error_code& ec) {
    DoSSFStart(ec);
  };
  this->DoSSFInitiate(*p_socket_, on_ssf_initiate);
}

template <class N, template <class> class T>
void Session<N, T>::DoSSFStart(const boost::system::error_code& ec) {
  boost::system::error_code stop_ec;

  if (ec) {
    SSF_LOG(kLogError) << "[client][session] SSF protocol error: "
                       << ec.message();
    UpdateStatus(Status::kServerNotSupported);
    return;
  }

  SSF_LOG(kLogTrace) << "[client][session] SSF reply ok";
  boost::system::error_code fiberize_ec;
  DoFiberize(fiberize_ec);
  if (fiberize_ec) {
    UpdateStatus(Status::kServerNotSupported);
    return;
  }
}

template <class N, template <class> class T>
void Session<N, T>::DoFiberize(boost::system::error_code& ec) {
  auto self = this->shared_from_this();
  auto close_demux_handler = [this, self]() { OnDemuxClose(); };
  fiber_demux_.fiberize(std::move(*p_socket_), close_demux_handler);

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
  services::process::Server<Demux>::RegisterToServiceFactory(
      p_service_factory, services_config_.process());

  // Create admin microservice
  auto on_user_service_status = [this, self](
      BaseUserServicePtr p_service, const boost::system::error_code& ec) {
    on_user_service_status_(p_service, ec);
  };

  auto p_admin_service =
      services::admin::Admin<Demux>::Create(io_service_, fiber_demux_, {});
  p_admin_service->SetAsClient(user_services_, on_user_service_status);

  // Register supported admin microservice commands
  if (!p_admin_service->template RegisterCommand<
          services::admin::CreateServiceRequest>()) {
    SSF_LOG(kLogError) << "[client][session] cannot register "
                          "CreateServiceRequest into admin service";
    ec.assign(::error::service_not_started, ::error::get_ssf_category());
    return;
  }
  if (!p_admin_service
           ->template RegisterCommand<services::admin::StopServiceRequest>()) {
    SSF_LOG(kLogError) << "[client][session] cannot register "
                          "StopServiceRequest into admin service";
    ec.assign(::error::service_not_started, ::error::get_ssf_category());
    return;
  }
  if (!p_admin_service
           ->template RegisterCommand<services::admin::ServiceStatus>()) {
    SSF_LOG(kLogError) << "[client][session] cannot register "
                          "ServiceStatus into admin service";
    ec.assign(::error::service_not_started, ::error::get_ssf_category());
    return;
  }

  // Start admin microservice
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

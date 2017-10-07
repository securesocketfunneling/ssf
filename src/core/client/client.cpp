#include "core/client/client.h"

#include <ssf/log/log.h>

#include "common/error/error.h"

#include "services/admin/admin.h"
#include "services/admin/requests/create_service_request.h"
#include "services/admin/requests/service_status.h"
#include "services/admin/requests/stop_service_request.h"

#include "services/copy_file/fiber_to_file/fiber_to_file.h"
#include "services/copy_file/file_enquirer/file_enquirer.h"
#include "services/copy_file/file_to_fiber/file_to_fiber.h"
#include "services/datagrams_to_fibers/datagrams_to_fibers.h"
#include "services/fibers_to_datagrams/fibers_to_datagrams.h"
#include "services/fibers_to_sockets/fibers_to_sockets.h"
#include "services/process/server.h"
#include "services/sockets_to_fibers/sockets_to_fibers.h"
#include "services/socks/socks_server.h"

#include "services/user_services/base_user_service.h"

namespace ssf {

Client::Client()
    : async_engine_(),
      connection_attempts_(1),
      max_connection_attempts_(1),
      reconnection_timeout_(0),
      timer_(async_engine_.get_io_service()),
      stopped_(false) {}

Client::~Client() {
  if (session_) {
    boost::system::error_code stop_ec;
    session_->Stop(stop_ec);
  }
  Deinit();
}

void Client::Init(const NetworkQuery& network_query,
                  uint32_t max_connection_attempts,
                  uint32_t reconnection_timeout, bool no_reconnection,
                  UserServiceParameters user_service_params,
                  const ssf::config::Services& user_services_config,
                  OnStatusCb on_status,
                  OnUserServiceStatusCb on_user_service_status,
                  boost::system::error_code& ec) {
  SSF_LOG(kLogDebug) << "client: init";
  if (async_engine_.IsStarted()) {
    ec.assign(::error::device_or_resource_busy, ::error::get_ssf_category());
    SSF_LOG(kLogError) << "client: already initialized";
    return;
  }

  max_connection_attempts_ = max_connection_attempts;
  reconnection_timeout_ = std::chrono::seconds(reconnection_timeout);
  no_reconnection_ = no_reconnection;
  user_service_params_ = user_service_params;
  user_services_config_ = user_services_config;
  network_query_ = network_query;
  on_status_ = on_status;
  on_user_service_status_ = on_user_service_status;

  // try to generate user services
  CreateUserServices(ec);
  if (ec) {
    return;
  }

  async_engine_.Start();
}

void Client::Run(boost::system::error_code& ec) { RunSession(ec); }

void Client::WaitStop(boost::system::error_code& ec) {
  std::unique_lock<std::mutex> lock(stop_mutex_);
  cv_wait_stop_.wait(lock, [this] { return stopped_; });
  lock.unlock();
}

void Client::Stop(boost::system::error_code& ec) {
  SSF_LOG(kLogDebug) << "client: stop";

  {
    std::lock_guard<std::mutex> lock(stop_mutex_);
    if (stopped_) {
      return;
    }
    stopped_ = true;
  }

  timer_.cancel(ec);
  if (ec) {
    ec.clear();
  }
  if (session_) {
    session_->Stop(ec);
    session_.reset();
  }
  if (ec) {
    SSF_LOG(kLogDebug) << "client: error while closing session: "
                       << ec.message();
  }

  cv_wait_stop_.notify_all();
}

void Client::Deinit() {
  SSF_LOG(kLogDebug) << "client: deinit";
  async_engine_.Stop();
}

boost::asio::io_service& Client::get_io_service() {
  return async_engine_.get_io_service();
}

Client::UserServices Client::CreateUserServices(boost::system::error_code& ec) {
  UserServices user_services;
  // create CLI services (socks, port forwarding)
  for (const auto& service : user_service_params_) {
    for (const auto& service_param : service.second) {
      boost::system::error_code parse_ec;
      auto user_service = user_service_factory_.CreateUserService(
          service.first, service_param, parse_ec);
      if (!parse_ec) {
        user_services.push_back(user_service);
      } else {
        SSF_LOG(kLogError) << "client: invalid option value for service<"
                           << service.first << "> (" << parse_ec.message()
                           << ")";
        ec.assign(parse_ec.value(), parse_ec.category());
      }
    }
  }

  return user_services;
}

void Client::AsyncWaitReconnection() {
  if (connection_attempts_ > max_connection_attempts_ || no_reconnection_) {
    async_engine_.get_io_service().post([this]() {
      boost::system::error_code stop_ec;
      Stop(stop_ec);
    });
    return;
  }

  SSF_LOG(kLogInfo) << "client: wait " << reconnection_timeout_.count()
                    << "s before reconnection";
  timer_.expires_from_now(reconnection_timeout_);
  timer_.async_wait(
      std::bind(&Client::RunSession, this, std::placeholders::_1));
}

void Client::RunSession(const boost::system::error_code& ec) {
  {
    std::lock_guard<std::mutex> lock(stop_mutex_);
    if (ec || stopped_ || connection_attempts_ > max_connection_attempts_) {
      async_engine_.get_io_service().post([this]() {
        boost::system::error_code stop_ec;
        Stop(stop_ec);
      });
      return;
    }
  }

  SSF_LOG(kLogInfo) << "client: connection attempt " << connection_attempts_
                    << "/" << max_connection_attempts_;

  ++connection_attempts_;

  boost::system::error_code create_session_ec;
  auto user_services = CreateUserServices(create_session_ec);
  if (create_session_ec) {
    return;
  }

  auto on_session_status =
      std::bind(&Client::OnSessionStatus, this, std::placeholders::_1);

  auto on_user_service_status =
      std::bind(&Client::OnUserServiceStatus, this, std::placeholders::_1,
                std::placeholders::_2);

  session_ = ClientSession::Create(
      async_engine_.get_io_service(), user_services, user_services_config_,
      on_session_status, on_user_service_status, create_session_ec);

  if (create_session_ec) {
    return;
  }

  session_->Start(network_query_, create_session_ec);
  if (create_session_ec) {
    boost::system::error_code stop_ec;
    session_->Stop(stop_ec);
    return;
  }
}

void Client::OnSessionStatus(Status status) {
  boost::system::error_code stop_ec;

  on_status_(status);

  if (stopped_) {
    return;
  }

  switch (status) {
    case Status::kEndpointNotResolvable:
      SSF_LOG(kLogInfo) << "client: endpoint not resolvable";
      if (session_) {
        session_->Stop(stop_ec);
      }
      break;
    case Status::kServerUnreachable:
      SSF_LOG(kLogInfo) << "client: server unreachable";
      if (session_) {
        session_->Stop(stop_ec);
      }
      AsyncWaitReconnection();
      break;
    case Status::kServerNotSupported:
      SSF_LOG(kLogInfo) << "client: server not supported";
      if (session_) {
        session_->Stop(stop_ec);
      }
      AsyncWaitReconnection();
      break;
    case Status::kConnected:
      SSF_LOG(kLogInfo) << "client: connected to server";
      break;
    case Status::kDisconnected:
      SSF_LOG(kLogInfo) << "client: disconnected";
      if (session_) {
        session_->Stop(stop_ec);
      }
      AsyncWaitReconnection();
      break;
    case Status::kRunning:
      // reset connection attempts
      connection_attempts_ = 1;
      SSF_LOG(kLogInfo) << "client: running";
      break;
    default:
      break;
  }
}

void Client::OnUserServiceStatus(UserServicePtr user_service,
                                 const boost::system::error_code& ec) {
  SSF_LOG(kLogInfo) << "client: on service status";
  if (user_service == nullptr) {
    return;
  }

  if (ec) {
    SSF_LOG(kLogError) << "client: service <" << user_service->GetName()
                       << "> KO";
  } else {
    SSF_LOG(kLogInfo) << "client: service <" << user_service->GetName()
                      << "> OK";
  }
  on_user_service_status_(user_service, ec);
}

}  // ssf

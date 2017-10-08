#ifndef SSF_CORE_CLIENT_CLIENT_H_
#define SSF_CORE_CLIENT_CLIENT_H_

#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <map>
#include <mutex>
#include <string>

#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>
#include "common/config/config.h"

#include "core/async_engine.h"
#include "core/network_protocol.h"
#include "core/client/session.h"
#include "core/client/status.h"
#include "core/transport_virtual_layer_policies/transport_protocol_policy.h"

#include "services/user_services/base_user_service.h"
#include "services/user_service_factory.h"
#include "services/user_services/parameters.h"

namespace ssf {

class Client {
 private:
  using ClientSession =
      Session<network::NetworkProtocol::Protocol, TransportProtocolPolicy>;
  using ClientSessionPtr = ClientSession::SessionPtr;

 public:
  using NetworkSocket = ClientSession::NetworkSocket;
  using NetworkQuery = ClientSession::NetworkQuery;
  using Demux = ClientSession::Demux;

  using UserServiceFactory = ssf::UserServiceFactory<Demux>;
  using UserServicePtr = UserServiceFactory::UserServicePtr;
  using UserServices = std::vector<UserServicePtr>;

  using OnStatusCb = ClientSession::OnStatusCb;
  using OnUserServiceStatusCb = ClientSession::OnUserServiceStatusCb;

 public:
  Client();

  ~Client();

  template <class UserService>
  void Register() {
    user_service_factory_.Register<UserService>();
  }

  void Init(const NetworkQuery& network_query, uint32_t max_connection_attempts,
            uint32_t reconnection_timeout,
            bool no_reconnection,
            UserServiceParameters user_service_params,
            const ssf::config::Services& user_services_config,
            OnStatusCb on_status, OnUserServiceStatusCb on_user_service_status,
            boost::system::error_code& ec);

  // Run
  void Run(boost::system::error_code& ec);

  // Wait until max connection attempts reached or client is stopped
  void WaitStop(boost::system::error_code& ec);

  void Stop(boost::system::error_code& ec);

  void Deinit();

  boost::asio::io_service& get_io_service();

 private:
  UserServices CreateUserServices(boost::system::error_code& ec);
  void AsyncWaitReconnection();
  void RunSession(const boost::system::error_code& ec);
  void OnSessionStatus(Status status);
  void OnUserServiceStatus(UserServicePtr user_service,
                           const boost::system::error_code& ec);

 private:
  AsyncEngine async_engine_;
  NetworkQuery network_query_;
  UserServiceFactory user_service_factory_;
  UserServiceParameters user_service_params_;
  ssf::config::Services user_services_config_;
  uint32_t connection_attempts_;
  uint32_t max_connection_attempts_;
  bool no_reconnection_;
  std::chrono::seconds reconnection_timeout_;
  OnStatusCb on_status_;
  OnUserServiceStatusCb on_user_service_status_;
  boost::asio::steady_timer timer_;
  ClientSessionPtr session_;
  std::condition_variable cv_wait_stop_;
  std::mutex stop_mutex_;
  bool stopped_;
};

}  // ssf

#endif  // SSF_CORE_CLIENT_CLIENT_H_

#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/system/error_code.hpp>

#include <ssf/log/log.h>

#include "common/config/config.h"
#include "common/log/log.h"

#include "core/client/client.h"
#include "core/client/client_helper.h"
#include "core/command_line/standard/command_line.h"
#include "core/command_line/user_service_option_factory.h"

#include "services/user_services/base_user_service.h"
#include "services/user_services/parameters.h"
#include "services/user_services/socks.h"
#include "services/user_services/remote_socks.h"
#include "services/user_services/port_forwarding.h"
#include "services/user_services/remote_port_forwarding.h"
#include "services/user_services/process.h"
#include "services/user_services/remote_process.h"
#include "services/user_services/udp_port_forwarding.h"
#include "services/user_services/udp_remote_port_forwarding.h"

using Demux = ssf::Client::Demux;

// register supported user services (port forwarding, SOCKS, shell...)
void RegisterUserServices(
    ssf::Client* client,
    ssf::UserServiceOptionFactory* user_service_option_factory);

int main(int argc, char** argv) {
  boost::system::error_code ec;

  ssf::Client client;
  ssf::UserServiceOptionFactory user_service_option_factory;
  ssf::UserServiceParameters user_service_parameters;

  RegisterUserServices(&client, &user_service_option_factory);

  // CLI options
  ssf::command_line::StandardCommandLine cmd;

  user_service_parameters =
      cmd.Parse(argc, argv, user_service_option_factory, ec);
  if (ec.value() == ::error::operation_canceled) {
    return 0;
  } else if (ec) {
    SSF_LOG(kLogError) << "ssfc: invalid command line arguments";
    return 1;
  }

  // read configuration file
  ssf::config::Config ssf_config;
  ssf_config.Init();
  ssf_config.UpdateFromFile(cmd.config_file(), ec);
  if (ec) {
    SSF_LOG(kLogError) << "ssfc: invalid config file format";
    return 1;
  }

  if (ssf_config.GetArgc() > 0) {
    // update command line with config file argv
    user_service_parameters =
        cmd.Parse(ssf_config.GetArgc(), ssf_config.GetArgv().data(),
                  user_service_option_factory, ec);
    if (ec) {
      SSF_LOG(kLogError) << "ssfc: invalid command line arguments";
      return 1;
    }
  }

  ssf_config.Log();
  if (cmd.show_status()) {
    ssf_config.LogStatus();
  }

  ssf::log::Configure(cmd.log_level());

  if (!cmd.host_set()) {
    SSF_LOG(kLogError) << "ssfc: no server host provided";
    return 1;
  }

  if (!cmd.port_set()) {
    SSF_LOG(kLogError) << "ssfc: no server port provided";
    return 1;
  }

  auto endpoint_query = ssf::GenerateNetworkQuery(
      cmd.host(), std::to_string(cmd.port()), ssf_config);

  ssf_config.services().SetGatewayPorts(cmd.gateway_ports());

  // initialize and run client
  auto on_status = [](ssf::Status) {};
  auto on_user_service_status =
      [](ssf::Client::UserServicePtr, const boost::system::error_code&) {};

  client.Init(endpoint_query, cmd.max_connection_attempts(),
              cmd.reconnection_timeout(), cmd.no_reconnection(),
              user_service_parameters, ssf_config.services(), on_status,
              on_user_service_status, ec);
  if (ec) {
    SSF_LOG(kLogError) << "[ssf] cannot init client";
    return 1;
  }

  SSF_LOG(kLogInfo) << "[ssf] connecting to <" << cmd.host() << ":"
                    << cmd.port() << ">";

  SSF_LOG(kLogInfo) << "[ssf] running (Ctrl + C to stop)";

  // stop client on SIGINT or SIGTERM
  boost::asio::signal_set signal(client.get_io_service(), SIGINT, SIGTERM);
  signal.async_wait([&client](const boost::system::error_code& ec, int signum) {
    if (ec) {
      return;
    }

    boost::system::error_code stop_ec;
    client.Stop(stop_ec);
  });

  client.Run(ec);
  if (ec) {
    SSF_LOG(kLogError) << "[ssf] error happened when running client: "
                       << ec.message();
    return 1;
  }

  // blocks until signal or max reconnection attempts
  client.WaitStop(ec);

  SSF_LOG(kLogInfo) << "[ssf] stop";
  signal.cancel(ec);

  return 0;
}

void RegisterUserServices(
    ssf::Client* client,
    ssf::UserServiceOptionFactory* user_service_option_factory) {
  // client user services
  client->Register<ssf::services::PortForwarding<Demux>>();
  client->Register<ssf::services::RemotePortForwarding<Demux>>();
  client->Register<ssf::services::Socks<Demux>>();
  client->Register<ssf::services::RemoteSocks<Demux>>();
  client->Register<ssf::services::UdpPortForwarding<Demux>>();
  client->Register<ssf::services::UdpRemotePortForwarding<Demux>>();
  client->Register<ssf::services::Process<Demux>>();
  client->Register<ssf::services::RemoteProcess<Demux>>();

  // user service CLI options
  user_service_option_factory->Register<ssf::services::PortForwarding<Demux>>();
  user_service_option_factory
      ->Register<ssf::services::RemotePortForwarding<Demux>>();
  user_service_option_factory->Register<ssf::services::Socks<Demux>>();
  user_service_option_factory->Register<ssf::services::RemoteSocks<Demux>>();
  user_service_option_factory
      ->Register<ssf::services::UdpPortForwarding<Demux>>();
  user_service_option_factory
      ->Register<ssf::services::UdpRemotePortForwarding<Demux>>();
  user_service_option_factory->Register<ssf::services::Process<Demux>>();
  user_service_option_factory->Register<ssf::services::RemoteProcess<Demux>>();
}

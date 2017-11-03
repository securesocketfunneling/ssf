#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/system/error_code.hpp>

#include <ssf/log/log.h>

#include "common/config/config.h"

#include "core/client/client.h"
#include "core/client/client_helper.h"
#include "core/command_line/standard/command_line.h"
#include "core/command_line/user_service_option_factory.h"

#include "services/user_services/base_user_service.h"
#include "services/user_services/parameters.h"
#include "services/user_services/port_forwarding.h"
#include "services/user_services/shell.h"
#include "services/user_services/remote_port_forwarding.h"
#include "services/user_services/remote_shell.h"
#include "services/user_services/remote_socks.h"
#include "services/user_services/socks.h"
#include "services/user_services/udp_port_forwarding.h"
#include "services/user_services/remote_udp_port_forwarding.h"

using Demux = ssf::Client::Demux;

// register supported user services (port forwarding, SOCKS, shell...)
void RegisterUserServices(
    ssf::Client* client,
    ssf::UserServiceOptionFactory* user_service_option_factory);

void Run(int argc, char** argv, boost::system::error_code& exit_ec);

int main(int argc, char** argv) {
  boost::system::error_code exit_ec;

  Run(argc, argv, exit_ec);

  SSF_LOG("ssf", debug, "exit {} ({})", exit_ec.value(), exit_ec.message());

  return exit_ec.value();
}

void Run(int argc, char** argv, boost::system::error_code& exit_ec) {
  ssf::Client client;
  ssf::UserServiceOptionFactory user_service_option_factory;
  ssf::UserServiceParameters user_service_parameters;

  RegisterUserServices(&client, &user_service_option_factory);

  // CLI options
  ssf::command_line::StandardCommandLine cmd;

  user_service_parameters =
      cmd.Parse(argc, argv, user_service_option_factory, exit_ec);
  if (exit_ec.value() == ::error::operation_canceled) {
    exit_ec.assign(::error::success, ::error::get_ssf_category());
    return;
  } else if (exit_ec) {
    SSF_LOG("ssf", error, "invalid command line arguments");
    return;
  }

  SetLogLevel(cmd.log_level());

  // read configuration file
  ssf::config::Config ssf_config;
  ssf_config.Init();
  ssf_config.UpdateFromFile(cmd.config_file(), exit_ec);
  if (exit_ec) {
    SSF_LOG("ssf", error, "invalid config file format");
    return;
  }

  if (ssf_config.GetArgc() > 0) {
    // update command line with config file argv
    user_service_parameters =
        cmd.Parse(ssf_config.GetArgc(), ssf_config.GetArgv().data(),
                  user_service_option_factory, exit_ec);
    if (exit_ec) {
      SSF_LOG("ssf", error, "invalid command line arguments");
      return;
    }
  }

  ssf_config.Log();
  if (cmd.show_status()) {
    ssf_config.LogStatus();
  }

  if (!cmd.host_set()) {
    SSF_LOG("ssf", error, "no server host provided");
    exit_ec.assign(::error::destination_address_required,
                   ::error::get_ssf_category());
    return;
  }

  if (!cmd.port_set()) {
    SSF_LOG("ssf", error, "no server port provided");
    exit_ec.assign(::error::destination_address_required,
                   ::error::get_ssf_category());
    return;
  }

  auto endpoint_query = ssf::GenerateNetworkQuery(
      cmd.host(), std::to_string(cmd.port()), ssf_config);

  ssf_config.services().SetGatewayPorts(cmd.gateway_ports());

  // initialize and run client
  auto on_status = [&client, &exit_ec](ssf::Status status) {
    switch (status) {
      case ssf::Status::kEndpointNotResolvable:
        exit_ec.assign(::error::address_not_available,
                       ::error::get_ssf_category());
        break;
      case ssf::Status::kServerUnreachable:
        exit_ec.assign(::error::host_unreachable, ::error::get_ssf_category());
        break;
      case ssf::Status::kConnected:
        exit_ec.assign(::error::success, ::error::get_ssf_category());
        break;
      case ssf::Status::kDisconnected:
        if (exit_ec.value() != ::error::interrupted) {
          exit_ec.assign(::error::broken_pipe, ::error::get_ssf_category());
        }
        break;
    }
  };
  auto on_user_service_status = [&client, &cmd, &exit_ec](
      ssf::Client::UserServicePtr service,
      const boost::system::error_code& ec) {};

  client.Init(endpoint_query, cmd.max_connection_attempts(),
              cmd.reconnection_timeout(), cmd.no_reconnection(),
              user_service_parameters, ssf_config.services(), on_status,
              on_user_service_status, exit_ec);
  if (exit_ec) {
    SSF_LOG("ssf", error, "cannot init client");
    return;
  }

  SSF_LOG("ssf", info, "connecting to <{}:{}>", cmd.host(), cmd.port());
  SSF_LOG("ssf", info, "running (Ctrl + C to stop)");

  // stop client on SIGINT or SIGTERM
  boost::asio::signal_set signal(client.get_io_service(), SIGINT, SIGTERM);
  signal.async_wait(
      [&client, &exit_ec](const boost::system::error_code& ec, int signum) {
        if (ec) {
          return;
        }

        exit_ec.assign(::error::interrupted, ::error::get_ssf_category());
        boost::system::error_code stop_ec;
        client.Stop(stop_ec);
      });

  client.Run(exit_ec);
  if (exit_ec) {
    SSF_LOG("ssf", error, "error happened when running client: {}",
            exit_ec.message());
    return;
  }

  // blocks until signal or max reconnection attempts
  boost::system::error_code stop_ec;
  client.WaitStop(stop_ec);

  SSF_LOG("ssf", debug, "stop");
  signal.cancel(stop_ec);

  client.Deinit();
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
  client->Register<ssf::services::RemoteUdpPortForwarding<Demux>>();
  client->Register<ssf::services::Shell<Demux>>();
  client->Register<ssf::services::RemoteShell<Demux>>();

  // user service CLI options
  user_service_option_factory->Register<ssf::services::PortForwarding<Demux>>();
  user_service_option_factory
      ->Register<ssf::services::RemotePortForwarding<Demux>>();
  user_service_option_factory->Register<ssf::services::Socks<Demux>>();
  user_service_option_factory->Register<ssf::services::RemoteSocks<Demux>>();
  user_service_option_factory
      ->Register<ssf::services::UdpPortForwarding<Demux>>();
  user_service_option_factory
      ->Register<ssf::services::RemoteUdpPortForwarding<Demux>>();
  user_service_option_factory->Register<ssf::services::Shell<Demux>>();
  user_service_option_factory->Register<ssf::services::RemoteShell<Demux>>();
}

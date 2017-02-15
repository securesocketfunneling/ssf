#include <condition_variable>
#include <mutex>

#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/system/error_code.hpp>

#include <ssf/log/log.h>

#include "common/config/config.h"
#include "common/log/log.h"

#include "core/client/client.h"
#include "core/client/client_helper.h"
#include "core/command_line/standard/command_line.h"
#include "core/factories/service_option_factory.h"
#include "core/network_protocol.h"
#include "core/transport_virtual_layer_policies/transport_protocol_policy.h"

#include "services/initialisation.h"
#include "services/user_services/base_user_service.h"
#include "services/user_services/socks.h"
#include "services/user_services/remote_socks.h"
#include "services/user_services/port_forwarding.h"
#include "services/user_services/remote_port_forwarding.h"
#include "services/user_services/process.h"
#include "services/user_services/remote_process.h"
#include "services/user_services/udp_port_forwarding.h"
#include "services/user_services/udp_remote_port_forwarding.h"

using NetworkProtocol = ssf::network::NetworkProtocol;

using Client =
    ssf::SSFClient<NetworkProtocol::Protocol, ssf::TransportProtocolPolicy>;

using Demux = Client::Demux;
using BaseUserServicePtr = Client::BaseUserServicePtr;
using ClientServices = std::vector<BaseUserServicePtr>;
using ParsedParameters =
    ssf::command_line::standard::CommandLine::ParsedParameters;

// register supported client services (port forwarding, SOCKS, shell...)
void RegisterSupportedClientServices();

// initialize request client services from parameters
void InitializeClientServices(ClientServices* p_client_services,
                              const ParsedParameters& parameters,
                              boost::system::error_code& ec);

int main(int argc, char** argv) {
  RegisterSupportedClientServices();

  // generate cli options from supported services
  boost::program_options::options_description options =
      ssf::ServiceOptionFactory<Demux>::GetOptionDescriptions();

  ssf::command_line::standard::CommandLine cmd;

  boost::system::error_code ec;
  ParsedParameters parameters;

  ssf::config::Config ssf_config;
  ssf_config.Init();

  parameters = cmd.Parse(argc, argv, options, ec);
  if (ec.value() == ::error::operation_canceled) {
    return 0;
  } else if (ec) {
    SSF_LOG(kLogError) << "ssfc: wrong command line arguments";
    return 1;
  }

  ssf_config.UpdateFromFile(cmd.config_file(), ec);
  if (ec) {
    SSF_LOG(kLogError) << "ssfc: invalid config file format";
    return 1;
  }

  if (ssf_config.GetArgc() > 0) {
    // update command line with config file argv
    parameters = cmd.Parse(ssf_config.GetArgc(), ssf_config.GetArgv().data(),
                           options, ec);
    if (ec) {
      SSF_LOG(kLogError) << "ssfc: wrong command line arguments";
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

  std::condition_variable wait_stop_cv;
  std::mutex mutex;
  bool stopped = false;

  auto callback = [&wait_stop_cv, &mutex, &stopped](
      ssf::services::initialisation::type type, BaseUserServicePtr p_service,
      const boost::system::error_code& ec) {
    switch (type) {
      case ssf::services::initialisation::NETWORK: {
        if (ec) {
          SSF_LOG(kLogError) << "ssfc: connected to remote server NOK";
          {
            boost::lock_guard<std::mutex> lock(mutex);
            stopped = true;
          }
          wait_stop_cv.notify_all();
        } else {
          SSF_LOG(kLogInfo) << "ssfc: connected to remote server OK";
        }
        break;
      }
      case ssf::services::initialisation::SERVICE: {
        if (p_service.get() != nullptr) {
          if (ec) {
            SSF_LOG(kLogError) << "ssfc: service <" << p_service->GetName()
                               << "> KO";
          } else {
            SSF_LOG(kLogInfo) << "ssfc: service <" << p_service->GetName()
                              << "> OK";
          }
        }
        break;
      }
      case ssf::services::initialisation::CLOSE: {
        SSF_LOG(kLogInfo) << "ssfc: connection closed";
        {
          boost::lock_guard<std::mutex> lock(mutex);
          stopped = true;
        }
        wait_stop_cv.notify_all();
      }
      default:
        break;
    }
  };

  ssf_config.services().SetGatewayPorts(cmd.gateway_ports());

  ClientServices user_services;
  InitializeClientServices(&user_services, parameters, ec);
  if (ec) {
    SSF_LOG(kLogError)
        << "ssfc: initialization of client services failed -- Exiting";
    return 1;
  }

  // initialize and run client
  Client client(user_services, ssf_config.services(), callback);

  client.Run(endpoint_query, ec);

  if (ec) {
    SSF_LOG(kLogError) << "ssfc: error happened when running client: "
                       << ec.message();
    return 1;
  }

  SSF_LOG(kLogInfo) << "ssfc: connecting to <" << cmd.host() << ":"
                    << cmd.port() << ">";

  boost::asio::signal_set signal(client.get_io_service(), SIGINT, SIGTERM);

  signal.async_wait([&wait_stop_cv, &mutex, &stopped](
      const boost::system::error_code& ec, int signum) {
    if (ec) {
      return;
    }
    {
      boost::lock_guard<std::mutex> lock(mutex);
      stopped = true;
    }
    wait_stop_cv.notify_all();
  });

  SSF_LOG(kLogInfo) << "ssfc: running (Ctrl + C to stop)";

  std::unique_lock<std::mutex> lock(mutex);
  wait_stop_cv.wait(lock, [&stopped] { return stopped; });
  lock.unlock();

  SSF_LOG(kLogInfo) << "ssfc: stop";
  signal.cancel(ec);
  client.Stop();

  return 0;
}

void RegisterSupportedClientServices() {
  ssf::services::Socks<Demux>::RegisterToServiceOptionFactory();
  ssf::services::RemoteSocks<Demux>::RegisterToServiceOptionFactory();
  ssf::services::PortForwarding<Demux>::RegisterToServiceOptionFactory();
  ssf::services::RemotePortForwarding<Demux>::RegisterToServiceOptionFactory();
  ssf::services::UdpPortForwarding<Demux>::RegisterToServiceOptionFactory();
  ssf::services::UdpRemotePortForwarding<
      Demux>::RegisterToServiceOptionFactory();
  ssf::services::Process<Demux>::RegisterToServiceOptionFactory();
  ssf::services::RemoteProcess<Demux>::RegisterToServiceOptionFactory();
}

void InitializeClientServices(ClientServices* p_client_services,
                              const ParsedParameters& parameters,
                              boost::system::error_code& ec) {
  // initialize requested user services (socks, port forwarding)
  for (const auto& parameter : parameters) {
    for (const auto& single_parameter : parameter.second) {
      boost::system::error_code parse_ec;
      auto p_service_options =
          ssf::ServiceOptionFactory<Demux>::ParseServiceLine(
              parameter.first, single_parameter, parse_ec);
      if (!parse_ec) {
        p_client_services->push_back(p_service_options);
      } else {
        SSF_LOG(kLogError) << "ssfc: invalid option value for service<"
                           << parameter.first << ">: " << single_parameter
                           << " (" << parse_ec.message() << ")";
        ec.assign(parse_ec.value(), parse_ec.category());
      }
    }
  }
}

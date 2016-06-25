#include <condition_variable>
#include <mutex>

#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/system/error_code.hpp>

#include <ssf/log/log.h>

#include "common/config/config.h"
#include "common/log/log.h"

#include "core/circuit/config.h"
#include "core/client/client.h"
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
using CircuitNodeList = ssf::circuit::NodeList;
using CircuitConfig = ssf::circuit::Config;
using ParsedParameters =
    ssf::command_line::standard::CommandLine::ParsedParameters;

// Register the supported client services (port forwarding, SOCKS)
void RegisterSupportedClientServices();

// Initialize request client services from parameters
void InitializeClientServices(ClientServices* p_client_services,
                              const ParsedParameters& parameters,
                              boost::system::error_code& ec);

// Generate network query
NetworkProtocol::Query GenerateNetworkQuery(
    const std::string& remote_addr, const std::string& remote_port,
    const ssf::config::Config& config, const CircuitConfig& circuit_config);

int main(int argc, char** argv) {
  ssf::log::Configure();

  RegisterSupportedClientServices();

  // Generate options description from supported services
  boost::program_options::options_description options =
      ssf::ServiceOptionFactory<Demux>::GetOptionDescriptions();

  ssf::command_line::standard::CommandLine cmd;

  boost::system::error_code ec;
  ParsedParameters parameters = cmd.parse(argc, argv, options, ec);

  if (ec.value() == ::error::operation_canceled) {
    return 0;
  }

  if (ec) {
    SSF_LOG(kLogError) << "client: wrong command line arguments";
    return 1;
  }

  if (!cmd.IsAddrSet()) {
    SSF_LOG(kLogError) << "client: no hostname provided -- Exiting";
    return 1;
  }

  if (!cmd.IsPortSet()) {
    SSF_LOG(kLogError) << "client: no host port provided -- Exiting";
    return 1;
  }

  ClientServices user_services;
  InitializeClientServices(&user_services, parameters, ec);

  if (ec) {
    SSF_LOG(kLogError)
        << "client: initialization of client services failed -- Exiting";
    return 1;
  }

  // Load SSF config if any
  ssf::config::Config ssf_config;

  ssf_config.Update(cmd.config_file(), ec);

  if (ec) {
    SSF_LOG(kLogError) << "client: invalid config file format -- Exiting";
    return 1;
  }

  ssf_config.Log();

  CircuitConfig circuit_config;
  circuit_config.Update(cmd.circuit_file(), ec);

  if (ec) {
    SSF_LOG(kLogError) << "client: invalid circuit config file -- Exiting";
    return 1;
  }

  circuit_config.Log();

  auto endpoint_query = GenerateNetworkQuery(
      cmd.addr(), std::to_string(cmd.port()), ssf_config, circuit_config);

  std::condition_variable wait_stop_cv;
  std::mutex mutex;
  bool stopped = false;

  auto callback = [&wait_stop_cv, &mutex, &stopped](
      ssf::services::initialisation::type type, BaseUserServicePtr p_service,
      const boost::system::error_code& ec) {
    switch (type) {
      case ssf::services::initialisation::NETWORK: {
        if (ec) {
          SSF_LOG(kLogError) << "client: connected to remote server NOK";
          {
            boost::lock_guard<std::mutex> lock(mutex);
            stopped = true;
          }
          wait_stop_cv.notify_all();
        } else {
          SSF_LOG(kLogInfo) << "client: connected to remote server OK";
        }
        break;
      }
      case ssf::services::initialisation::SERVICE: {
        if (p_service.get() != nullptr) {
          if (ec) {
            SSF_LOG(kLogError) << "client: service <" << p_service->GetName()
                               << "> NOK";
          } else {
            SSF_LOG(kLogInfo) << "client: service <" << p_service->GetName()
                              << "> OK";
          }
        }
        break;
      }
      case ssf::services::initialisation::CLOSE: {
        SSF_LOG(kLogInfo) << "client: connection closed";
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

  // Initiate and run client
  Client client(user_services, ssf_config.services(), callback);

  client.Run(endpoint_query, ec);

  if (ec) {
    SSF_LOG(kLogError) << "client: error happened when running client : "
                       << ec.message();
    return 1;
  }

  SSF_LOG(kLogInfo) << "client: connecting to <" << cmd.addr() << ":"
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

  SSF_LOG(kLogInfo) << "client: running (Ctrl + C to stop)";

  std::unique_lock<std::mutex> lock(mutex);
  wait_stop_cv.wait(lock, [&stopped] { return stopped; });
  lock.unlock();

  SSF_LOG(kLogInfo) << "client: stop";
  signal.cancel(ec);
  client.Stop();

  return 0;
}

void RegisterSupportedClientServices() {
  ssf::services::Socks<Demux>::RegisterToServiceOptionFactory();
  ssf::services::RemoteSocks<Demux>::RegisterToServiceOptionFactory();
  ssf::services::PortForwading<Demux>::RegisterToServiceOptionFactory();
  ssf::services::RemotePortForwading<Demux>::RegisterToServiceOptionFactory();
  ssf::services::UdpPortForwading<Demux>::RegisterToServiceOptionFactory();
  ssf::services::UdpRemotePortForwading<
      Demux>::RegisterToServiceOptionFactory();
  ssf::services::Process<Demux>::RegisterToServiceOptionFactory();
  ssf::services::RemoteProcess<Demux>::RegisterToServiceOptionFactory();
}

void InitializeClientServices(ClientServices* p_client_services,
                              const ParsedParameters& parameters,
                              boost::system::error_code& ec) {
  // Initialize requested user services (socks, port forwarding)
  for (const auto& parameter : parameters) {
    for (const auto& single_parameter : parameter.second) {
      auto p_service_options =
          ssf::ServiceOptionFactory<Demux>::ParseServiceLine(
              parameter.first, single_parameter, ec);

      if (!ec) {
        p_client_services->push_back(p_service_options);
      } else {
        SSF_LOG(kLogError) << "client: wrong parameter " << parameter.first
                           << ": " << single_parameter << ": " << ec.message();
        return;
      }
    }
  }
}

NetworkProtocol::Query GenerateNetworkQuery(
    const std::string& remote_addr, const std::string& remote_port,
    const ssf::config::Config& ssf_config,
    const CircuitConfig& circuit_config) {
  std::string first_node_addr;
  std::string first_node_port;
  CircuitNodeList nodes = circuit_config.nodes();
  if (nodes.size()) {
    auto first_node = nodes.front();
    nodes.pop_front();
    first_node_addr = first_node.addr();
    first_node_port = first_node.port();
    nodes.emplace_back(remote_addr, remote_port);
  } else {
    first_node_addr = remote_addr;
    first_node_port = remote_port;
  }

  return NetworkProtocol::GenerateClientQuery(first_node_addr, first_node_port,
                                              ssf_config, nodes);
}

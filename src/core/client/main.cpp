#include <boost/asio/io_service.hpp>
#include <boost/system/error_code.hpp>

#include "common/config/config.h"
#include "common/log/log.h"

#include "core/client/client.h"
#include "core/command_line/standard/command_line.h"
#include "core/factories/service_option_factory.h"
#include "core/network_protocol.h"
#include "core/parser/bounce_parser.h"
#include "core/transport_virtual_layer_policies/transport_protocol_policy.h"

#include "services/initialisation.h"
#include "services/user_services/base_user_service.h"
#include "services/user_services/socks.h"
#include "services/user_services/remote_socks.h"
#include "services/user_services/port_forwarding.h"
#include "services/user_services/remote_port_forwarding.h"
#include "services/user_services/udp_port_forwarding.h"
#include "services/user_services/udp_remote_port_forwarding.h"

using Client =
    ssf::SSFClient<ssf::network::Protocol, ssf::TransportProtocolPolicy>;

using Demux = Client::Demux;
using BaseUserServicePtr = Client::BaseUserServicePtr;
using ClientServices = std::vector<BaseUserServicePtr>;
using CircuitBouncers = std::list<std::string>;
using BounceParser = ssf::parser::BounceParser;
using ParsedParameters =
    ssf::command_line::standard::CommandLine::ParsedParameters;

// Register the supported client services (port forwarding, SOCKS)
void RegisterSupportedClientServices();

// Initialize request client services from parameters
void InitializeClientServices(ClientServices* p_client_services,
                              const ParsedParameters& parameters,
                              boost::system::error_code& ec);

// Generate network query
ssf::network::Query GenerateNetworkQuery(const std::string& remote_addr,
                                         const std::string& remote_port,
                                         const ssf::Config& config,
                                         const CircuitBouncers& bouncers);

int main(int argc, char** argv) {
  ssf::log::Configure();

  RegisterSupportedClientServices();

  // Generate options description from supported services
  boost::program_options::options_description options =
      ssf::ServiceOptionFactory<Demux>::GetOptionDescriptions();

  ssf::command_line::standard::CommandLine cmd;

  boost::system::error_code ec;
  ParsedParameters parameters = cmd.parse(argc, argv, options, ec);

  if (ec) {
    BOOST_LOG_TRIVIAL(error) << "client: wrong command line arguments";
    return 1;
  }

  if (!cmd.IsAddrSet()) {
    BOOST_LOG_TRIVIAL(error) << "client: no hostname provided -- Exiting";
    return 1;
  }

  if (!cmd.IsPortSet()) {
    BOOST_LOG_TRIVIAL(error) << "client: no host port provided -- Exiting";
    return 1;
  }

  ClientServices user_services;
  InitializeClientServices(&user_services, parameters, ec);

  if (ec) {
    BOOST_LOG_TRIVIAL(error)
        << "client: initialization of client services failed -- Exiting";
    return 1;
  }

  // Load SSF config if any
  ssf::Config ssf_config = ssf::LoadConfig(cmd.config_file(), ec);

  if (ec) {
    BOOST_LOG_TRIVIAL(error) << "client: invalid config file format -- Exiting";
    return 1;
  }

  auto callback = [](ssf::services::initialisation::type type,
                     BaseUserServicePtr p_service,
                     const boost::system::error_code& ec) {
    switch (type) {
      case ssf::services::initialisation::NETWORK:
        BOOST_LOG_TRIVIAL(info) << "client: connected to remote server "
                                << (!ec ? "OK" : "NOK");
        break;
      case ssf::services::initialisation::SERVICE:
        if (p_service.get() != nullptr) {
          BOOST_LOG_TRIVIAL(info) << "client: service " << p_service->GetName()
                                  << " " << (!ec ? "OK" : "NOK");
        }
        break;
      default:
        break;
    }
  };

  // Initiate and run client
  Client client(user_services, callback);

  CircuitBouncers bouncers = BounceParser::ParseBounceFile(cmd.bounce_file());

  auto endpoint_query = GenerateNetworkQuery(
      cmd.addr(), std::to_string(cmd.port()), ssf_config, bouncers);

  client.Run(endpoint_query, ec);

  if (!ec) {
    BOOST_LOG_TRIVIAL(info) << "client: connecting to " << cmd.addr() << ":"
                            << cmd.port();
    BOOST_LOG_TRIVIAL(info) << "client: press [ENTER] to stop";
    getchar();
  } else {
    BOOST_LOG_TRIVIAL(error)
        << "client: error happened when running client : " << ec.message();
  }

  BOOST_LOG_TRIVIAL(info) << "client: stop" << std::endl;
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
        BOOST_LOG_TRIVIAL(error) << "client: wrong parameter "
                                 << parameter.first << ": " << single_parameter
                                 << ": " << ec.message();
        return;
      }
    }
  }
}

ssf::network::Query GenerateNetworkQuery(const std::string& remote_addr,
                                         const std::string& remote_port,
                                         const ssf::Config& ssf_config,
                                         const CircuitBouncers& bouncers) {
  std::string first_node_addr;
  std::string first_node_port;
  CircuitBouncers nodes = bouncers;
  if (nodes.size()) {
    auto first = nodes.front();
    nodes.pop_front();
    first_node_addr = BounceParser::GetRemoteAddress(first);
    first_node_port = BounceParser::GetRemotePort(first);
    nodes.push_back(remote_addr + ":" + remote_port);
  } else {
    first_node_addr = remote_addr;
    first_node_port = remote_port;
  }

  return ssf::network::GenerateClientQuery(first_node_addr, first_node_port,
                                           ssf_config, nodes);
}

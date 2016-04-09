#include <stdexcept>

#include <boost/asio/io_service.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
#include <boost/system/error_code.hpp>

#include <ssf/layer/data_link/circuit_helpers.h>
#include <ssf/layer/data_link/basic_circuit_protocol.h>
#include <ssf/layer/data_link/simple_circuit_policy.h>
#include <ssf/layer/parameters.h>
#include <ssf/layer/physical/tcp.h>
#include <ssf/layer/physical/tlsotcp.h>

#include "common/config/config.h"

#include "core/client/client.h"

#include "core/command_line/standard/command_line.h"
#include "core/parser/bounce_parser.h"
#include "core/factories/service_option_factory.h"
#include "core/transport_virtual_layer_policies/transport_protocol_policy.h"

#include "core/client/query_factory.h"

#include "services/initialisation.h"
#include "services/user_services/base_user_service.h"
#include "services/user_services/socks.h"
#include "services/user_services/remote_socks.h"
#include "services/user_services/port_forwarding.h"
#include "services/user_services/remote_port_forwarding.h"
#include "services/user_services/udp_port_forwarding.h"
#include "services/user_services/udp_remote_port_forwarding.h"

void Init() {
  boost::log::core::get()->set_filter(boost::log::trivial::severity >=
                                      boost::log::trivial::info);
}

int main(int argc, char** argv) {
  using CircuitBouncers = std::list<std::string>;
  using NetworkQuery = ssf::layer::ParameterStack;

#ifdef TLS_OVER_TCP_LINK
  using TLSPhysicalProtocol = ssf::layer::physical::TLSboTCPPhysicalLayer;
  using TLSCircuitProtocol = ssf::layer::data_link::basic_CircuitProtocol<
      TLSPhysicalProtocol, ssf::layer::data_link::CircuitPolicy>;

  using Client =
      ssf::SSFClient<TLSCircuitProtocol, ssf::TransportProtocolPolicy>;

#elif TCP_ONLY_LINK
  using PhysicalProtocol = ssf::layer::physical::TCPPhysicalLayer;
  using CircuitProtocol = ssf::layer::data_link::basic_CircuitProtocol<
      PhysicalProtocol, ssf::layer::data_link::CircuitPolicy>;

  using Client =
      ssf::SSFClient<CircuitProtocol, ssf::TransportProtocolPolicy>;
#endif

  using demux = Client::demux;
  using BaseUserServicePtr =
      ssf::services::BaseUserService<demux>::BaseUserServicePtr;
  using BounceParser = ssf::parser::BounceParser;

  Init();

  // Register user services supported
  ssf::services::Socks<demux>::RegisterToServiceOptionFactory();
  ssf::services::RemoteSocks<demux>::RegisterToServiceOptionFactory();
  ssf::services::PortForwading<demux>::RegisterToServiceOptionFactory();
  ssf::services::RemotePortForwading<demux>::RegisterToServiceOptionFactory();
  ssf::services::UdpPortForwading<demux>::RegisterToServiceOptionFactory();
  ssf::services::UdpRemotePortForwading<
      demux>::RegisterToServiceOptionFactory();

  boost::program_options::options_description options =
      ssf::ServiceOptionFactory<demux>::GetOptionDescriptions();

  // Parse the command line
  ssf::command_line::standard::CommandLine cmd;
  std::vector<BaseUserServicePtr> user_services;

  boost::system::error_code ec;
  auto parameters = cmd.parse(argc, argv, options, ec);

  if (ec) {
    BOOST_LOG_TRIVIAL(error) << "client: wrong command line arguments";
    return 1;
  }

  // Initialize requested user services (socks, port forwarding)
  for (const auto& parameter : parameters) {
    for (const auto& single_parameter : parameter.second) {
      boost::system::error_code ec;

      auto p_service_options =
          ssf::ServiceOptionFactory<demux>::ParseServiceLine(
              parameter.first, single_parameter, ec);

      if (!ec) {
        user_services.push_back(p_service_options);
      } else {
        BOOST_LOG_TRIVIAL(error) << "client: wrong parameter "
                                 << parameter.first << " : " << single_parameter
                                 << " : " << ec.message();
      }
    }
  }

  if (!cmd.IsAddrSet()) {
    BOOST_LOG_TRIVIAL(error) << "client: no hostname provided -- Exiting";
    return 1;
  }

  if (!cmd.IsPortSet()) {
    BOOST_LOG_TRIVIAL(error) << "client: no host port provided -- Exiting";
    return 1;
  }

  // Load SSF config if any
  boost::system::error_code ec_config;
  ssf::Config ssf_config = ssf::LoadConfig(cmd.config_file(), ec_config);

  if (ec_config) {
    BOOST_LOG_TRIVIAL(error) << "client: invalid config file format"
                             << std::endl;
    return 0;
  }

  // Initialize the asynchronous engine
  boost::asio::io_service io_service;
  boost::thread_group threads;

  auto callback = [](ssf::services::initialisation::type, BaseUserServicePtr,
                     const boost::system::error_code&) {};

  // Initiate and start client
  Client client(io_service, user_services, callback);

  CircuitBouncers bouncers = BounceParser::ParseBounceFile(cmd.bounce_file());

  std::string remote_addr;
  std::string remote_port;
  if (bouncers.size()) {
    auto first = bouncers.front();
    bouncers.pop_front();
    remote_addr = BounceParser::GetRemoteAddress(first);
    remote_port = BounceParser::GetRemotePort(first);
    bouncers.push_back(cmd.addr() + ":" + std::to_string(cmd.port()));
  } else {
    remote_addr = cmd.addr();
    remote_port = std::to_string(cmd.port());
  }

  // construct endpoint parameter stack
#ifdef TLS_OVER_TCP_LINK
  auto endpoint_query =
      ssf::GenerateTLSNetworkQuery(remote_addr, remote_port, ssf_config, bouncers);
#elif TCP_ONLY_LINK
  auto endpoint_query =
      ssf::GenerateTCPNetworkQuery(remote_addr, remote_port, bouncers);
#endif

  client.Run(endpoint_query);

  for (uint8_t i = 0; i < boost::thread::hardware_concurrency(); ++i) {
    auto lambda = [&]() {
      boost::system::error_code ec;
      try {
        io_service.run(ec);
      } catch (std::exception e) {
        BOOST_LOG_TRIVIAL(error) << "client: exception in io_service run "
                                 << e.what();
      }
    };

    threads.create_thread(lambda);
  }

  getchar();

  client.Stop();
  threads.join_all();
  io_service.stop();

  return 0;
}

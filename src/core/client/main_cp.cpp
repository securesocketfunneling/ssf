#include <future>

#include <boost/asio/io_service.hpp>
#include <boost/system/error_code.hpp>

#include "common/config/config.h"
#include "common/log/log.h"

#include "core/client/client.h"
#include "core/command_line/copy/command_line.h"
#include "core/factories/service_option_factory.h"
#include "core/network_protocol.h"
#include "core/parser/bounce_parser.h"
#include "core/transport_virtual_layer_policies/transport_protocol_policy.h"

#include "services/initialisation.h"
#include "services/user_services/base_user_service.h"
#include "services/user_services/copy_file_service.h"

using Client =
    ssf::SSFClient<ssf::network::Protocol, ssf::TransportProtocolPolicy>;

using Demux = Client::Demux;
using BaseUserServicePtr = Client::BaseUserServicePtr;
using CircuitBouncers = std::list<std::string>;
using BounceParser = ssf::parser::BounceParser;

// Generate network query
ssf::network::Query GenerateNetworkQuery(const std::string& remote_addr,
                                         const std::string& remote_port,
                                         const ssf::Config& config,
                                         const CircuitBouncers& bouncers);

int main(int argc, char** argv) {
  ssf::log::Configure();

  // Parse the command line
  ssf::command_line::copy::CommandLine cmd;

  boost::system::error_code ec;
  cmd.parse(argc, argv, ec);

  if (ec) {
    BOOST_LOG_TRIVIAL(error) << "client: wrong command line arguments";
    return 1;
  }

  // Create and initialize copy user service
  auto p_copy_service =
      ssf::services::CopyFileService<Demux>::CreateServiceFromParams(
          cmd.from_stdin(), cmd.from_local_to_remote(), cmd.input_pattern(),
          cmd.output_pattern(), ec);

  if (ec) {
    BOOST_LOG_TRIVIAL(error) << "client: copy service could not be created";
    return 1;
  }

  std::vector<BaseUserServicePtr> user_services;
  user_services.push_back(p_copy_service);

  if (!cmd.IsAddrSet()) {
    BOOST_LOG_TRIVIAL(error) << "client: no remote host provided -- Exiting";
    return 1;
  }

  if (!cmd.IsPortSet()) {
    BOOST_LOG_TRIVIAL(error) << "client: no host port provided -- Exiting";
    return 1;
  }

  boost::system::error_code ec_config;
  ssf::Config ssf_config = ssf::LoadConfig(cmd.config_file(), ec_config);

  if (ec_config) {
    BOOST_LOG_TRIVIAL(error) << "client: invalid config file format";
    return 1;
  }

  std::promise<bool> closed;

  auto callback =
      [&closed](ssf::services::initialisation::type type, BaseUserServicePtr,
                const boost::system::error_code& ec) {
        switch (type) {
          case ssf::services::initialisation::NETWORK:
            BOOST_LOG_TRIVIAL(info) << "client: connected to remote server "
                                    << (!ec ? "OK" : "NOK");
            break;
          case ssf::services::initialisation::CLOSE:
            closed.set_value(true);
            return;
          default:
            break;
        }
        if (ec) {
          closed.set_value(true);
        }
      };

  // Initiating and starting the client
  Client client(user_services, callback);

  CircuitBouncers bouncers = BounceParser::ParseBounceFile(cmd.bounce_file());

  auto endpoint_query = GenerateNetworkQuery(
      cmd.addr(), std::to_string(cmd.port()), ssf_config, bouncers);

  boost::system::error_code run_ec;
  client.Run(endpoint_query, run_ec);

  if (!run_ec) {
    BOOST_LOG_TRIVIAL(info) << "client: connecting to " << cmd.addr() << ":"
                            << cmd.port();
    // wait end transfer
    BOOST_LOG_TRIVIAL(info) << "client: wait end of file transfer";
    closed.get_future().get();
  } else {
    BOOST_LOG_TRIVIAL(error)
        << "client: error happened when running client : " << run_ec.message();
  }

  client.Stop();

  return 0;
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

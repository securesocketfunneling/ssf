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
#include "core/command_line/copy/command_line.h"
#include "core/factories/service_option_factory.h"
#include "core/network_protocol.h"
#include "core/transport_virtual_layer_policies/transport_protocol_policy.h"

#include "services/initialisation.h"
#include "services/user_services/base_user_service.h"
#include "services/user_services/copy_file_service.h"

using NetworkProtocol = ssf::network::NetworkProtocol;
using Client =
    ssf::SSFClient<NetworkProtocol::Protocol, ssf::TransportProtocolPolicy>;

using Demux = Client::Demux;
using BaseUserServicePtr = Client::BaseUserServicePtr;
using CircuitNodeList = ssf::circuit::NodeList;
using CircuitConfig = ssf::circuit::Config;

// Generate network query
NetworkProtocol::Query GenerateNetworkQuery(
    const std::string& remote_addr, const std::string& remote_port,
    const ssf::config::Config& config, const CircuitConfig& circuit_config);

int main(int argc, char** argv) {
  // Parse the command line
  ssf::command_line::copy::CommandLine cmd;

  boost::system::error_code ec;
  cmd.Parse(argc, argv, ec);

  if (ec.value() == ::error::operation_canceled) {
    return 0;
  }

  if (ec) {
    SSF_LOG(kLogError) << "client: wrong command line arguments";
    return 1;
  }
  
  ssf::log::Configure(cmd.log_level());

  // Create and initialize copy user service
  auto p_copy_service =
      ssf::services::CopyFileService<Demux>::CreateServiceFromParams(
          cmd.from_stdin(), cmd.from_local_to_remote(), cmd.input_pattern(),
          cmd.output_pattern(), ec);

  if (ec) {
    SSF_LOG(kLogError) << "client: copy service could not be created";
    return 1;
  }

  std::vector<BaseUserServicePtr> user_services;
  user_services.push_back(p_copy_service);

  if (!cmd.host_set()) {
    SSF_LOG(kLogError) << "client: no remote host provided -- Exiting";
    return 1;
  }

  if (!cmd.port_set()) {
    SSF_LOG(kLogError) << "client: no host port provided -- Exiting";
    return 1;
  }

  boost::system::error_code ec_config;
  ssf::config::Config ssf_config;
  ssf_config.Update(cmd.config_file(), ec_config);

  if (ec_config) {
    SSF_LOG(kLogError) << "client: invalid config file format";
    return 1;
  }

  ssf_config.Log();

  std::promise<bool> closed;

  CircuitConfig circuit_config;
  circuit_config.Update(cmd.circuit_file(), ec);

  if (ec) {
    SSF_LOG(kLogError) << "client: invalid circuit config file -- Exiting";
    return 1;
  }

  circuit_config.Log();

  auto endpoint_query = GenerateNetworkQuery(
      cmd.host(), std::to_string(cmd.port()), ssf_config, circuit_config);

  std::condition_variable wait_stop_cv;
  std::mutex mutex;
  bool stopped = false;

  auto callback = [&wait_stop_cv, &mutex, &stopped](
      ssf::services::initialisation::type type, BaseUserServicePtr,
      const boost::system::error_code& ec) {
    switch (type) {
      case ssf::services::initialisation::NETWORK: {
        SSF_LOG(kLogInfo) << "client: connected to remote server "
                          << (!ec ? "OK" : "NOK");
        break;
      }
      case ssf::services::initialisation::CLOSE: {
        {
          boost::lock_guard<std::mutex> lock(mutex);
          stopped = true;
        }
        wait_stop_cv.notify_all();
        return;
      }
      default:
        break;
    }
    if (ec) {
      {
        boost::lock_guard<std::mutex> lock(mutex);
        stopped = true;
      }
      wait_stop_cv.notify_all();
    }
  };

  // Initiating and starting the client
  Client client(user_services, ssf_config.services(), callback);

  client.Run(endpoint_query, ec);

  if (ec) {
    SSF_LOG(kLogError) << "client: error happened when running client : "
                       << ec.message();
    return 1;
  }

  SSF_LOG(kLogInfo) << "client: connecting to <" << cmd.host() << ":"
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

  // wait end transfer
  SSF_LOG(kLogInfo) << "client: wait end of file transfer";
  std::unique_lock<std::mutex> lock(mutex);
  wait_stop_cv.wait(lock, [&stopped] { return stopped; });
  lock.unlock();
  signal.cancel(ec);

  client.Stop();

  return 0;
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

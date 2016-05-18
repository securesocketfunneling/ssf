#include <future>

#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/system/error_code.hpp>

#include <ssf/log/log.h>

#include "common/config/config.h"
#include "common/log/log.h"

#include "core/command_line/standard/command_line.h"
#include "core/network_protocol.h"
#include "core/server/server.h"
#include "core/transport_virtual_layer_policies/transport_protocol_policy.h"

using NetworkProtocol = ssf::network::NetworkProtocol;

using Server =
    ssf::SSFServer<NetworkProtocol::Protocol, ssf::TransportProtocolPolicy>;

int main(int argc, char** argv) {
  ssf::log::Configure();

  // The command line parser
  ssf::command_line::standard::CommandLine cmd(true);

  // Parse the command line
  boost::system::error_code ec;
  cmd.parse(argc, argv, ec);

  if (ec) {
    SSF_LOG(kLogError) << "server: wrong arguments -- Exiting";
    return 1;
  }

  // Load SSF config if any
  ssf::config::Config ssf_config;
  ssf_config.Update(cmd.config_file(), ec);

  if (ec) {
    SSF_LOG(kLogError) << "server: invalid config file format -- Exiting";
    return 1;
  }

  ssf_config.Log();

  // Initiate and start the server
  Server server;

  // construct endpoint parameter stack
  auto endpoint_query = NetworkProtocol::GenerateServerQuery(
      cmd.addr(), std::to_string(cmd.port()), ssf_config);

  SSF_LOG(kLogInfo) << "server: listening on port " << cmd.port();

  server.Run(endpoint_query, ec);

  if (ec) {
    SSF_LOG(kLogError) << "server: error happened when running server: "
                       << ec.message();
    return 1;
  }

  std::promise<bool> stopped;
  boost::asio::signal_set signal(server.get_io_service(), SIGINT, SIGTERM);

  signal.async_wait(
      [&stopped](const boost::system::error_code& ec, int signum) {
        if (ec) {
          return;
        }

        stopped.set_value(true);
      });

  SSF_LOG(kLogInfo) << "server: running (Ctrl + C to stop)";
  std::future<bool> stop = stopped.get_future();
  stop.wait();

  SSF_LOG(kLogInfo) << "server: stop";
  server.Stop();

  return 0;
}
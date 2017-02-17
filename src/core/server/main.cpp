#include <condition_variable>
#include <mutex>

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
  ssf::command_line::standard::CommandLine cmd(true);

  ssf::config::Config ssf_config;
  ssf_config.Init();

  // parse command line
  boost::system::error_code ec;
  cmd.Parse(argc, argv, ec);

  if (ec.value() == ::error::operation_canceled) {
    return 0;
  } else if (ec) {
    SSF_LOG(kLogError) << "ssfs: wrong command line arguments";
    return 1;
  }

  // load config file
  ssf_config.UpdateFromFile(cmd.config_file(), ec);
  if (ec) {
    SSF_LOG(kLogError) << "ssfs: invalid config file format";
    return 1;
  }

  if (ssf_config.GetArgc() > 0) {
    // update command line with config file argv
    cmd.Parse(ssf_config.GetArgc(), ssf_config.GetArgv().data(), ec);
    if (ec) {
      SSF_LOG(kLogError) << "ssfs: wrong command line arguments";
      return 1;
    }
  }

  ssf::log::Configure(cmd.log_level());

  ssf_config.Log();
  if (cmd.show_status()) {
    ssf_config.LogStatus();
  }

  ssf_config.services().SetGatewayPorts(cmd.gateway_ports());

  // initialize and run the server
  Server server(ssf_config.services(), cmd.relay_only());

  // construct endpoint parameter stack
  auto endpoint_query = NetworkProtocol::GenerateServerQuery(
      cmd.host(), std::to_string(cmd.port()), ssf_config);

  SSF_LOG(kLogInfo) << "ssfs: listening on <"
                    << (!cmd.host().empty() ? cmd.host() : "*") << ":"
                    << cmd.port() << ">";

  server.Run(endpoint_query, ec);

  if (ec) {
    SSF_LOG(kLogError) << "ssfs: error happened when running server: "
                       << ec.message();
    return 1;
  }

  std::condition_variable wait_stop_cv;
  std::mutex mutex;
  bool stopped = false;
  boost::asio::signal_set signal(server.get_io_service(), SIGINT, SIGTERM);

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

  SSF_LOG(kLogInfo) << "ssfs: running (Ctrl + C to stop)";

  std::unique_lock<std::mutex> lock(mutex);
  wait_stop_cv.wait(lock, [&stopped] { return stopped; });
  lock.unlock();

  SSF_LOG(kLogInfo) << "ssfs: stop";
  signal.cancel(ec);
  server.Stop();

  return 0;
}
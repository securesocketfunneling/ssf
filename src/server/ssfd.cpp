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

void Run(int argc, char** argv, boost::system::error_code& exit_ec);

int main(int argc, char** argv) {
  boost::system::error_code exit_ec;

  Run(argc, argv, exit_ec);

  SSF_LOG(kLogInfo) << "[ssfd] exit " << exit_ec.value() << " ("
                    << exit_ec.message() << ")";

  return exit_ec.value();
}

void Run(int argc, char** argv, boost::system::error_code& exit_ec) {
  boost::system::error_code stop_ec;

  ssf::command_line::StandardCommandLine cmd(true);

  ssf::config::Config ssf_config;
  ssf_config.Init();

  // parse command line
  cmd.Parse(argc, argv, exit_ec);

  if (exit_ec.value() == ::error::operation_canceled) {
    exit_ec.assign(::error::success, ::error::get_ssf_category());
    return;
  } else if (exit_ec) {
    SSF_LOG(kLogError) << "[ssfd] wrong command line arguments";
    return;
  }

  ssf::log::Configure(cmd.log_level());

  // load config file
  ssf_config.UpdateFromFile(cmd.config_file(), exit_ec);
  if (exit_ec) {
    SSF_LOG(kLogError) << "[ssfd] invalid config file format";
    return;
  }

  if (ssf_config.GetArgc() > 0) {
    // update command line with config file argv
    cmd.Parse(ssf_config.GetArgc(), ssf_config.GetArgv().data(), exit_ec);
    if (exit_ec) {
      SSF_LOG(kLogError) << "[ssfd] invalid command line arguments";
      return;
    }
  }

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

  SSF_LOG(kLogInfo) << "[ssfd] listening on <"
                    << (!cmd.host().empty() ? cmd.host() : "*") << ":"
                    << cmd.port() << ">";

  server.Run(endpoint_query, exit_ec);

  if (exit_ec) {
    SSF_LOG(kLogError) << "[ssfd] error happened when running server: "
                       << exit_ec.message();
    return;
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

    SSF_LOG(kLogInfo) << "[ssfd] interrupted";

    {
      std::lock_guard<std::mutex> lock(mutex);
      stopped = true;
    }
    wait_stop_cv.notify_all();
  });

  SSF_LOG(kLogInfo) << "[ssfd] running (Ctrl + C to stop)";

  std::unique_lock<std::mutex> lock(mutex);
  wait_stop_cv.wait(lock, [&stopped] { return stopped; });
  lock.unlock();

  SSF_LOG(kLogInfo) << "[ssfd] stop";
  signal.cancel(stop_ec);
  server.Stop();

  return;
}
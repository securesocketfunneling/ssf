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
#include "core/command_line/copy/command_line.h"
#include "core/factories/service_option_factory.h"
#include "core/network_protocol.h"
#include "core/transport_virtual_layer_policies/transport_protocol_policy.h"

#include "services/user_services/base_user_service.h"
#include "services/user_services/copy_file_service.h"

using Demux = ssf::Client::Demux;
using CopyFileService = ssf::services::CopyFileService<Demux>;

int main(int argc, char** argv) {
  boost::system::error_code ec;

  ssf::Client client;

  client.Register<CopyFileService>();

  ssf::config::Config ssf_config;
  ssf_config.Init();

  // CLI options
  ssf::command_line::CopyCommandLine cmd;
  cmd.Parse(argc, argv, ec);
  if (ec.value() == ::error::operation_canceled) {
    return 0;
  } else if (ec) {
    SSF_LOG(kLogError) << "ssfcp: invalid command line arguments";
    return 1;
  }
  // read file configuration file
  ssf_config.UpdateFromFile(cmd.config_file(), ec);
  if (ec) {
    SSF_LOG(kLogError) << "ssfcp: invalid config file format";
    return 1;
  }

  if (ssf_config.GetArgc() > 0) {
    // update command line with config file argv
    cmd.Parse(ssf_config.GetArgc(), ssf_config.GetArgv().data(), ec);
    if (ec) {
      SSF_LOG(kLogError) << "ssfcp: invalid command line arguments";
      return 1;
    }
  }

  ssf_config.services().mutable_file_copy()->set_enabled(true);

  ssf_config.Log();
  ssf::log::Configure(cmd.log_level());

  // create and initialize copy user service
  ssf::UserServiceParameters copy_params = {
      {CopyFileService::GetParseName(),
       {CopyFileService::CreateUserServiceParameters(
           cmd.from_stdin(), cmd.from_local_to_remote(), cmd.input_pattern(),
           cmd.output_pattern(), ec)}}};
  if (ec) {
    SSF_LOG(kLogError)
        << "ssfcp: copy file service parameters could not be generated";
    return 1;
  }

  if (!cmd.host_set()) {
    SSF_LOG(kLogError) << "ssfcp: no remote host provided";
    return 1;
  }

  if (!cmd.port_set()) {
    SSF_LOG(kLogError) << "ssfcp: no host port provided";
    return 1;
  }

  auto endpoint_query = ssf::GenerateNetworkQuery(
      cmd.host(), std::to_string(cmd.port()), ssf_config);

  // initialize and run client
  auto on_status = [](ssf::Status) {};
  auto on_user_service_status =
      [](ssf::Client::UserServicePtr, const boost::system::error_code&) {};

  client.Init(endpoint_query, 1, 0, true, copy_params, ssf_config.services(),
              on_status, on_user_service_status, ec);
  if (ec) {
    SSF_LOG(kLogError) << "ssfcp: cannot init client";
    return 1;
  }

  SSF_LOG(kLogInfo) << "ssfcp: connecting to <" << cmd.host() << ":"
                    << cmd.port() << ">";

  SSF_LOG(kLogInfo) << "ssfcp: running (Ctrl + C to stop)";

  // stop client on SIGINT or SIGTERM
  boost::asio::signal_set signal(client.get_io_service(), SIGINT, SIGTERM);
  signal.async_wait([&client](const boost::system::error_code& ec, int signum) {
    if (ec) {
      return;
    }
    boost::system::error_code stop_ec;
    client.Stop(stop_ec);
  });

  client.Run(ec);
  if (ec) {
    SSF_LOG(kLogError) << "ssfcp: error happened when running client: "
                       << ec.message();
    return 1;
  }

  // blocks until signal or copy ends
  client.WaitStop(ec);

  SSF_LOG(kLogInfo) << "ssfcp: stop";
  signal.cancel(ec);

  return 0;
}

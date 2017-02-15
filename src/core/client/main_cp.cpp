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

#include "services/initialisation.h"
#include "services/user_services/base_user_service.h"
#include "services/user_services/copy_file_service.h"

using NetworkProtocol = ssf::network::NetworkProtocol;
using Client =
    ssf::SSFClient<NetworkProtocol::Protocol, ssf::TransportProtocolPolicy>;

using Demux = Client::Demux;
using BaseUserServicePtr = Client::BaseUserServicePtr;

int main(int argc, char** argv) {
  ssf::command_line::copy::CommandLine cmd;

  boost::system::error_code ec;

  ssf::config::Config ssf_config;
  ssf_config.Init();

  cmd.Parse(argc, argv, ec);
  if (ec.value() == ::error::operation_canceled) {
    return 0;
  } else if (ec) {
    SSF_LOG(kLogError) << "ssfcp: wrong command line arguments";
    return 1;
  }

  ssf_config.UpdateFromFile(cmd.config_file(), ec);
  if (ec) {
    SSF_LOG(kLogError) << "ssfcp: invalid config file format";
    return 1;
  }

  if (ssf_config.GetArgc() > 0) {
    // update command line with config file argv
    cmd.Parse(ssf_config.GetArgc(), ssf_config.GetArgv().data(), ec);
    if (ec) {
      SSF_LOG(kLogError) << "ssfcp: wrong command line arguments";
      return 1;
    }
  }

  ssf_config.Log();
  ssf::log::Configure(cmd.log_level());

  // create and initialize copy user service
  auto p_copy_service =
      ssf::services::CopyFileService<Demux>::CreateServiceFromParams(
          cmd.from_stdin(), cmd.from_local_to_remote(), cmd.input_pattern(),
          cmd.output_pattern(), ec);

  if (ec) {
    SSF_LOG(kLogError) << "ssfcp: copy service could not be created";
    return 1;
  }

  std::vector<BaseUserServicePtr> user_services;
  user_services.push_back(p_copy_service);

  if (!cmd.host_set()) {
    SSF_LOG(kLogError) << "ssfcp: no remote host provided";
    return 1;
  }

  if (!cmd.port_set()) {
    SSF_LOG(kLogError) << "ssfcp: no host port provided";
    return 1;
  }

  std::promise<bool> closed;

  auto endpoint_query = ssf::GenerateNetworkQuery(
      cmd.host(), std::to_string(cmd.port()), ssf_config);

  std::condition_variable wait_stop_cv;
  std::mutex mutex;
  bool stopped = false;

  auto callback = [&wait_stop_cv, &mutex, &stopped](
      ssf::services::initialisation::type type, BaseUserServicePtr,
      const boost::system::error_code& ec) {
    switch (type) {
      case ssf::services::initialisation::NETWORK: {
        SSF_LOG(kLogInfo) << "ssfcp: connected to remote server "
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

  // initialize and run client
  Client client(user_services, ssf_config.services(), callback);

  client.Run(endpoint_query, ec);

  if (ec) {
    SSF_LOG(kLogError) << "ssfcp: error happened when running client : "
                       << ec.message();
    return 1;
  }

  SSF_LOG(kLogInfo) << "ssfcp: connecting to <" << cmd.host() << ":"
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
  SSF_LOG(kLogInfo) << "ssfcp: wait end of file transfer";
  std::unique_lock<std::mutex> lock(mutex);
  wait_stop_cv.wait(lock, [&stopped] { return stopped; });
  lock.unlock();
  signal.cancel(ec);

  client.Stop();

  return 0;
}

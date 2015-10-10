#include <future>
#include <stdexcept>

#include <boost/asio/io_service.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
#include <boost/system/error_code.hpp>

#include "common/config/config.h"

#include "core/client/client.h"

#include "core/command_line/copy/command_line.h"
#include "core/parser/bounce_parser.h"
#include "core/factories/service_option_factory.h"
#include "core/network_virtual_layer_policies/bounce_protocol_policy.h"
#include "core/network_virtual_layer_policies/link_policies/ssl_policy.h"
#include "core/network_virtual_layer_policies/link_policies/tcp_policy.h"
#include "core/network_virtual_layer_policies/link_authentication_policies/null_link_authentication_policy.h"
#include "core/transport_virtual_layer_policies/transport_protocol_policy.h"

#include "services/initialisation.h"
#include "services/user_services/base_user_service.h"
#include "services/user_services/copy_file_service.h"

void Init() {
  boost::log::core::get()->set_filter(boost::log::trivial::severity >=
                                      boost::log::trivial::info);
}

int main(int argc, char** argv) {
#ifdef TLS_OVER_TCP_LINK
  using Client =
      ssf::SSFClient<ssf::SSLPolicy, ssf::NullLinkAuthenticationPolicy,
                     ssf::BounceProtocolPolicy, ssf::TransportProtocolPolicy>;
#elif TCP_ONLY_LINK
  using Client =
      ssf::SSFClient<ssf::TCPPolicy, ssf::NullLinkAuthenticationPolicy,
                     ssf::BounceProtocolPolicy, ssf::TransportProtocolPolicy>;
#endif

  using Demux = Client::demux;
  using BaseUserServicePtr =
      ssf::services::BaseUserService<Demux>::BaseUserServicePtr;
  using BounceParser = ssf::parser::BounceParser;

  Init();

  // Parse the command line
  ssf::command_line::copy::CommandLine cmd;
  std::vector<BaseUserServicePtr> user_services;

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

  user_services.push_back(p_copy_service);

  if (!cmd.IsAddrSet()) {
    BOOST_LOG_TRIVIAL(error) << "client: no remote host provided -- Exiting";
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
  std::unique_ptr<boost::asio::io_service::work> p_worker(
      new boost::asio::io_service::work(io_service));
  boost::thread_group threads;
  std::promise<bool> closed;

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

  auto callback =
      [&closed](ssf::services::initialisation::type type, BaseUserServicePtr,
                const boost::system::error_code&) {
        if (type == ssf::services::initialisation::CLOSE) {
          closed.set_value(true);
          return;
        }
      };

  // Initiating and starting the client
  Client client(io_service, cmd.addr(), std::to_string(cmd.port()), ssf_config,
                user_services, callback);

  std::map<std::string, std::string> params;

  std::list<std::string> bouncers =
      BounceParser::ParseBounceFile(cmd.bounce_file());

  if (bouncers.size()) {
    auto first = bouncers.front();
    bouncers.pop_front();
    params["remote_addr"] = BounceParser::GetRemoteAddress(first);
    params["remote_port"] = BounceParser::GetRemotePort(first);
    bouncers.push_back(cmd.addr() + ":" + std::to_string(cmd.port()));
  } else {
    params["remote_addr"] = cmd.addr();
    params["remote_port"] = std::to_string(cmd.port());
  }

  std::ostringstream ostrs;
  boost::archive::text_oarchive ar(ostrs);
  ar << BOOST_SERIALIZATION_NVP(bouncers);

  params["bouncing_nodes"] = ostrs.str();

  client.run(params);

  // wait end transfer
  closed.get_future().get();

  p_worker.reset();

  threads.join_all();

  client.stop();
  io_service.stop();

  return 0;
}

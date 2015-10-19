#include <stdexcept>

#include <boost/asio/io_service.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
#include <boost/system/error_code.hpp>

#include "common/config/config.h"

#include "core/client/client.h"

#include "core/command_line/standard/command_line.h"
#include "core/parser/bounce_parser.h"
#include "core/factories/service_option_factory.h"
#include "core/network_virtual_layer_policies/bounce_protocol_policy.h"
#include "core/network_virtual_layer_policies/link_policies/ssl_policy.h"
#include "core/network_virtual_layer_policies/link_policies/tcp_policy.h"
#include "core/network_virtual_layer_policies/link_authentication_policies/null_link_authentication_policy.h"
#include "core/transport_virtual_layer_policies/transport_protocol_policy.h"

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
#ifdef TLS_OVER_TCP_LINK
  using Client =
      ssf::SSFClient<ssf::SSLPolicy, ssf::NullLinkAuthenticationPolicy,
                     ssf::BounceProtocolPolicy, ssf::TransportProtocolPolicy>;
#elif TCP_ONLY_LINK
  using Client =
      ssf::SSFClient<ssf::TCPPolicy, ssf::NullLinkAuthenticationPolicy,
                     ssf::BounceProtocolPolicy, ssf::TransportProtocolPolicy>;
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
  boost::asio::io_service::work worker(io_service);
  boost::thread_group threads;

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

  auto callback = [](ssf::services::initialisation::type, BaseUserServicePtr,
                     const boost::system::error_code&) {};

  // Initiate and start client
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

  getchar();

  client.stop();
  threads.join_all();
  io_service.stop();

  return 0;
}

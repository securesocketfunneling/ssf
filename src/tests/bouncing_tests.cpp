#include <array>
#include <functional>
#include <future>
#include <memory>
#include <vector>

#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include "common/config/config.h"

#include "core/network_protocol.h"

#include "core/client/client.h"
#include "core/server/server.h"

#include "core/parser/bounce_parser.h"

#include "core/transport_virtual_layer_policies/transport_protocol_policy.h"

#include "services/initialisation.h"
#include "services/user_services/port_forwarding.h"

//-----------------------------------------------------------------------------
TEST(BouncingTests, BouncingChain) {
  using BounceParser = ssf::parser::BounceParser;
  using Client =
      ssf::SSFClient<ssf::network::Protocol, ssf::TransportProtocolPolicy>;
  using Server =
      ssf::SSFServer<ssf::network::Protocol, ssf::TransportProtocolPolicy>;

  using demux = Client::demux;
  using BaseUserServicePtr =
      ssf::services::BaseUserService<demux>::BaseUserServicePtr;

  boost::log::core::get()->set_filter(boost::log::trivial::severity >=
                                      boost::log::trivial::info);

  std::promise<bool> network_set;
  std::promise<bool> transport_set;
  std::promise<bool> service_set;

  std::list<Server> servers;
  std::list<std::string> bouncers;

  uint16_t initial_server_port = 10000;
  uint8_t nb_of_servers = 5;
  ssf::Config ssf_config;

  boost::system::error_code client_ec;
  boost::system::error_code bounce_ec;
  boost::system::error_code server_ec;

  ++initial_server_port;
  auto server_endpoint_query = ssf::network::GenerateServerQuery(
      "", std::to_string(initial_server_port), ssf_config);
  servers.emplace_front();
  servers.front().Run(server_endpoint_query, server_ec);
  ASSERT_EQ(server_ec.value(), 0) << "Server could not run";

  bouncers.emplace_front(std::string("127.0.0.1:") +
                         std::to_string(initial_server_port));

  for (uint8_t i = 0; i < nb_of_servers - 1; ++i) {
    ++initial_server_port;
    auto bounce_endpoint_query = ssf::network::GenerateServerQuery(
        "", std::to_string(initial_server_port), ssf_config);

    servers.emplace_front();
    servers.front().Run(bounce_endpoint_query, bounce_ec);
    ASSERT_EQ(server_ec.value(), 0) << "Bounce " << i << "could not run";
    bouncers.emplace_front(std::string("127.0.0.1:") +
                           std::to_string(initial_server_port));
  }

  std::vector<BaseUserServicePtr> client_options;
  boost::system::error_code client_option_ec;
  auto p_service = ssf::services::PortForwading<demux>::CreateServiceOptions(
      "5454:127.0.0.1:5354", client_option_ec);
  client_options.push_back(p_service);

  std::string error_msg;

  std::string remote_addr;
  std::string remote_port;

  auto first = bouncers.front();
  bouncers.pop_front();
  remote_addr = BounceParser::GetRemoteAddress(first);
  remote_port = BounceParser::GetRemotePort(first);

  auto callback = [&network_set, &transport_set, &service_set](
      ssf::services::initialisation::type type,
      BaseUserServicePtr p_user_service, const boost::system::error_code& ec) {
    if (type == ssf::services::initialisation::NETWORK) {
      network_set.set_value(!ec);
      if (ec) {
        service_set.set_value(false);
        transport_set.set_value(false);
      }

      return;
    }

    if (type == ssf::services::initialisation::TRANSPORT) {
      transport_set.set_value(!ec);
      if (ec) {
        service_set.set_value(false);
      }

      return;
    }

    if (type == ssf::services::initialisation::SERVICE) {
      service_set.set_value(!ec);
      return;
    }
  };

  auto client_endpoint_query = ssf::network::GenerateClientQuery(
      remote_addr, remote_port, ssf_config, bouncers);

  Client client(client_options, std::move(callback));
  client.Run(client_endpoint_query, client_ec);
  ASSERT_EQ(client_ec.value(), 0) << "Client could not run";

  auto network_future = network_set.get_future();
  auto transport_future = transport_set.get_future();
  auto service_future = service_set.get_future();

  network_future.wait();
  transport_future.wait();
  service_future.wait();

  ASSERT_TRUE(network_future.get()) << "Network should be set";
  ASSERT_TRUE(transport_future.get()) << "Transport should be set";
  ASSERT_TRUE(service_future.get()) << "Service should be set";

  client.Stop();
  for (auto& server : servers) {
    server.Stop();
  }

  servers.clear();
}
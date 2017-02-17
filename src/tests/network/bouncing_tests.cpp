#include <array>
#include <functional>
#include <future>
#include <memory>
#include <vector>

#include <boost/asio.hpp>

#include <gtest/gtest.h>

#include "common/config/config.h"

#include "core/network_protocol.h"

#include "core/client/client.h"
#include "core/server/server.h"

#include "core/transport_virtual_layer_policies/transport_protocol_policy.h"

#include "services/initialisation.h"
#include "services/user_services/port_forwarding.h"

using NetworkProtocol = ssf::network::NetworkProtocol;

TEST(BouncingTests, BouncingChain) {
  using Client =
      ssf::SSFClient<NetworkProtocol::Protocol, ssf::TransportProtocolPolicy>;
  using Server =
      ssf::SSFServer<NetworkProtocol::Protocol, ssf::TransportProtocolPolicy>;

  using demux = Client::Demux;
  using BaseUserServicePtr =
      ssf::services::BaseUserService<demux>::BaseUserServicePtr;

  std::promise<bool> network_set;
  std::promise<bool> transport_set;
  std::promise<bool> service_set;

  std::list<Server> servers;
  ssf::config::NodeList node_list;

  uint16_t initial_server_port = 11000;
  uint8_t nb_of_servers = 5;
  ssf::config::Config ssf_config;
  
  ssf_config.Init();

  boost::system::error_code client_ec;
  boost::system::error_code circuit_ec;
  boost::system::error_code server_ec;

  ++initial_server_port;
  auto server_endpoint_query = NetworkProtocol::GenerateServerQuery(
      "", std::to_string(initial_server_port), ssf_config);
  servers.emplace_front(ssf_config.services());
  servers.front().Run(server_endpoint_query, server_ec);
  ASSERT_EQ(server_ec.value(), 0) << "Server could not run";

  node_list.emplace_front("127.0.0.1", std::to_string(initial_server_port));

  for (uint8_t i = 0; i < nb_of_servers - 1; ++i) {
    ++initial_server_port;
    auto circuit_endpoint_query = NetworkProtocol::GenerateServerQuery(
        "", std::to_string(initial_server_port), ssf_config);

    servers.emplace_front(ssf_config.services());
    servers.front().Run(circuit_endpoint_query, circuit_ec);
    ASSERT_EQ(server_ec.value(), 0) << "Circuit node " << i << "could not run";
    node_list.emplace_front("127.0.0.1", std::to_string(initial_server_port));
  }

  std::vector<BaseUserServicePtr> client_options;
  boost::system::error_code client_option_ec;
  auto p_service = ssf::services::PortForwarding<demux>::CreateServiceOptions(
      "5454:127.0.0.1:5354", client_option_ec);
  client_options.push_back(p_service);

  std::string error_msg;

  std::string remote_addr;
  std::string remote_port;

  auto first_node = node_list.front();
  node_list.pop_front();

  auto callback = [&network_set, &transport_set, &service_set](
      ssf::services::initialisation::type type,
      BaseUserServicePtr p_user_service, const boost::system::error_code& ec) {
    if (type == ssf::services::initialisation::NETWORK) {
      network_set.set_value(!ec);

      EXPECT_EQ(ec.value(), 0) << "Error on network initialisation";
      if (ec) {
        service_set.set_value(false);
        transport_set.set_value(false);
      }

      return;
    }

    if (type == ssf::services::initialisation::TRANSPORT) {
      EXPECT_EQ(ec.value(), 0) << "Error on network initialisation";
      transport_set.set_value(!ec);
      if (ec) {
        service_set.set_value(false);
      }

      return;
    }

    if (type == ssf::services::initialisation::SERVICE) {
      EXPECT_EQ(ec.value(), 0) << "Error on network initialisation";
      service_set.set_value(!ec);
      return;
    }
  };

  auto client_endpoint_query = NetworkProtocol::GenerateClientQuery(
      first_node.addr(), first_node.port(), ssf_config, node_list);

  Client client(client_options, ssf_config.services(), std::move(callback));
  client.Run(client_endpoint_query, client_ec);
  ASSERT_EQ(client_ec.value(), 0) << "Client could not run";

  auto network_future = network_set.get_future();
  auto transport_future = transport_set.get_future();
  auto service_future = service_set.get_future();

  network_future.wait();
  transport_future.wait();
  service_future.wait();

  EXPECT_TRUE(network_future.get()) << "Network should be set";
  EXPECT_TRUE(transport_future.get()) << "Transport should be set";
  EXPECT_TRUE(service_future.get()) << "Service should be set";

  client.Stop();
  for (auto& server : servers) {
    server.Stop();
  }

  servers.clear();
}
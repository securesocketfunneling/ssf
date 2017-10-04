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

#include "services/user_services/port_forwarding.h"

using NetworkProtocol = ssf::network::NetworkProtocol;
using Client = ssf::Client;
using Server =
    ssf::SSFServer<NetworkProtocol::Protocol, ssf::TransportProtocolPolicy>;

using Demux = Client::Demux;
using UserServicePtr = Client::UserServicePtr;
using PortForwardingService = ssf::services::PortForwarding<Demux>;

TEST(CircuitTests, Basics) {
  std::promise<bool> network_set;
  std::promise<bool> transport_set;
  std::promise<bool> service_set;

  ssf::Client client;
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

  boost::system::error_code client_option_ec;
  ssf::UserServiceParameters service_params = {
      {PortForwardingService::GetParseName(),
       {PortForwardingService::CreateUserServiceParameters(
           "5454:127.0.0.1:5354", client_option_ec)}}};

  auto first_node = node_list.front();
  node_list.pop_front();

  auto on_client_status =
      [&network_set, &transport_set, &service_set](ssf::Status status) {
        switch (status) {
          case ssf::Status::kEndpointNotResolvable:
          case ssf::Status::kServerUnreachable:
            SSF_LOG(kLogCritical) << "Network initialization failed";
            network_set.set_value(false);
            transport_set.set_value(false);
            service_set.set_value(false);
            break;
          case ssf::Status::kServerNotSupported:
            SSF_LOG(kLogCritical) << "Transport initialization failed";
            transport_set.set_value(false);
            service_set.set_value(false);
            break;
          case ssf::Status::kConnected:
            network_set.set_value(true);
            break;
          case ssf::Status::kDisconnected:
            SSF_LOG(kLogInfo) << "client: disconnected";
            break;
          case ssf::Status::kRunning:
            transport_set.set_value(true);
            break;
          default:
            break;
        }
      };

  auto on_client_user_service_status = [&service_set](
      UserServicePtr p_user_service, const boost::system::error_code& ec) {
    if (ec) {
      SSF_LOG(kLogCritical) << "user_service[" << p_user_service->GetName()
                            << "]: initialization failed";
    }
    if (p_user_service->GetName() == PortForwardingService::GetParseName()) {
      service_set.set_value(!ec);
    }
  };

  auto client_endpoint_query = NetworkProtocol::GenerateClientQuery(
      first_node.addr(), first_node.port(), ssf_config, node_list);
  client.Register<PortForwardingService>();
  client.Init(client_endpoint_query, 1, 0, false, service_params,
              ssf_config.services(), on_client_status,
              on_client_user_service_status, client_ec);
  ASSERT_EQ(client_ec.value(), 0) << "Could not initialized client";

  client.Run(client_ec);
  ASSERT_EQ(client_ec.value(), 0) << "Could not run client";

  auto network_future = network_set.get_future();
  auto transport_future = transport_set.get_future();
  auto service_future = service_set.get_future();

  network_future.wait();
  transport_future.wait();
  service_future.wait();

  EXPECT_TRUE(network_future.get()) << "Network should be set";
  EXPECT_TRUE(transport_future.get()) << "Transport should be set";
  EXPECT_TRUE(service_future.get()) << "Service should be set";

  client.Stop(client_ec);
  for (auto& server : servers) {
    server.Stop();
  }

  servers.clear();
}
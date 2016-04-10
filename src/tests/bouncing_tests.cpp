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

#include <ssf/layer/data_link/circuit_helpers.h>
#include <ssf/layer/data_link/basic_circuit_protocol.h>
#include <ssf/layer/data_link/simple_circuit_policy.h>
#include <ssf/layer/parameters.h>
#include <ssf/layer/physical/tcp.h>
#include <ssf/layer/physical/tlsotcp.h>

#include "common/config/config.h"

#include "core/client/client.h"
#include "core/client/query_factory.h"
#include "core/server/server.h"
#include "core/server/query_factory.h"

#include "core/parser/bounce_parser.h"

#include "core/transport_virtual_layer_policies/transport_protocol_policy.h"

#include "services/initialisation.h"
#include "services/user_services/port_forwarding.h"

//-----------------------------------------------------------------------------
TEST(BouncingTests, BouncingChain) {
  using BounceParser = ssf::parser::BounceParser;
  using TLSPhysicalProtocol = ssf::layer::physical::TLSboTCPPhysicalLayer;
  using TLSCircuitProtocol = ssf::layer::data_link::basic_CircuitProtocol<
      TLSPhysicalProtocol, ssf::layer::data_link::CircuitPolicy>;
  using Client =
      ssf::SSFClient<TLSCircuitProtocol, ssf::TransportProtocolPolicy>;
  using Server =
      ssf::SSFServer<TLSCircuitProtocol, ssf::TransportProtocolPolicy>;
  using demux = Client::demux;
  using BaseUserServicePtr =
      ssf::services::BaseUserService<demux>::BaseUserServicePtr;

  boost::log::core::get()->set_filter(boost::log::trivial::severity >=
                                      boost::log::trivial::info);

  boost::recursive_mutex mutex;
  std::promise<bool> network_set;
  std::promise<bool> transport_set;
  std::promise<bool> service_set;

  boost::asio::io_service client_io_service;
  std::unique_ptr<boost::asio::io_service::work> p_client_worker(
    new boost::asio::io_service::work(client_io_service));
  boost::thread_group client_threads;
  for (uint8_t i = 1; i <= boost::thread::hardware_concurrency(); ++i) {
    client_threads.create_thread([&]() { client_io_service.run(); });
  }

  boost::asio::io_service bouncer_io_service;
  std::unique_ptr<boost::asio::io_service::work> p_bouncer_worker(
    new boost::asio::io_service::work(bouncer_io_service));
  boost::thread_group bouncer_threads;
  for (uint8_t i = 1; i <= boost::thread::hardware_concurrency(); ++i) {
    bouncer_threads.create_thread([&]() { bouncer_io_service.run(); });
  }

  boost::asio::io_service server_io_service;
  std::unique_ptr<boost::asio::io_service::work> p_server_worker(
    new boost::asio::io_service::work(server_io_service));
  boost::thread_group server_threads;
  for (uint8_t i = 1; i <= boost::thread::hardware_concurrency(); ++i) {
    server_threads.create_thread([&]() { server_io_service.run(); });
  }

  std::list<Server> servers;
  std::list<std::string> bouncers;

  uint16_t initial_server_port = 10000;
  uint8_t nb_of_servers = 10;

  {
    ssf::Config ssf_config;
    ++initial_server_port;
      auto endpoint_query = ssf::GenerateServerTLSNetworkQuery(
          std::to_string(initial_server_port), ssf_config);
    servers.emplace_front(server_io_service, ssf_config, initial_server_port);
    servers.front().Run(endpoint_query);
    bouncers.emplace_front(std::string("127.0.0.1:") +
      std::to_string(initial_server_port));
  }

    for (uint8_t i = 0; i < nb_of_servers - 1; ++i) {
      ssf::Config ssf_config;
      ++initial_server_port;
      auto endpoint_query = ssf::GenerateServerTLSNetworkQuery(
          std::to_string(initial_server_port), ssf_config);

      servers.emplace_front(bouncer_io_service, ssf_config, initial_server_port);
      servers.front().Run(endpoint_query);
      bouncers.emplace_front(std::string("127.0.0.1:") +
                             std::to_string(initial_server_port));
  }

    std::vector<BaseUserServicePtr> client_options;
    std::string error_msg;

    std::string remote_addr;
    std::string remote_port;

    auto first = bouncers.front();
    bouncers.pop_front();
    remote_addr = BounceParser::GetRemoteAddress(first);
    remote_port = BounceParser::GetRemotePort(first);

    ssf::Config ssf_config;

    auto callback = [&mutex, &network_set, &service_set, &transport_set](
        ssf::services::initialisation::type type,
        BaseUserServicePtr p_user_service,
        const boost::system::error_code& ec) {
      boost::recursive_mutex::scoped_lock lock(mutex);
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

        return;
      }
    };

    auto endpoint_query = ssf::GenerateTLSNetworkQuery(remote_addr, remote_port,
                                                       ssf_config, bouncers);

    Client client(client_io_service, client_options, std::move(callback));
    client.Run(endpoint_query);

    network_set.get_future().wait();
    transport_set.get_future().wait();

    { boost::recursive_mutex::scoped_lock lock(mutex); }

    client.Stop();
    for (auto& server : servers) {
      server.Stop();
    }

    p_client_worker.reset();
    client_threads.join_all();
    client_io_service.stop();

    p_bouncer_worker.reset();
    bouncer_threads.join_all();
    bouncer_io_service.stop();

    p_server_worker.reset();
    server_threads.join_all();
    server_io_service.stop();

    servers.clear();
}
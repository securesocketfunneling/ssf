#include <gtest/gtest.h>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include "common/config/config.h"

#include "core/network_protocol.h"
#include "core/transport_virtual_layer_policies/transport_protocol_policy.h"
#include "core/server/server.h"

TEST(SSFServerTest, failListeningWrongInterface) {
  boost::log::core::get()->set_filter(boost::log::trivial::severity >=
                                      boost::log::trivial::info);
  using Server =
      ssf::SSFServer<ssf::network::Protocol, ssf::TransportProtocolPolicy>;

  ssf::Config ssf_config;

  auto endpoint_query =
      ssf::network::GenerateServerQuery("1.1.1.1", "8000", ssf_config);
  Server server;

  boost::system::error_code run_ec;
  server.Run(endpoint_query, run_ec);

  ASSERT_NE(0, run_ec.value());
}

TEST(SSFServerTest, listeningAllInterfaces) {
  boost::log::core::get()->set_filter(boost::log::trivial::severity >=
                                      boost::log::trivial::info);

  using Server =
      ssf::SSFServer<ssf::network::Protocol, ssf::TransportProtocolPolicy>;

  ssf::Config ssf_config;

  auto endpoint_query =
      ssf::network::GenerateServerQuery("", "8000", ssf_config);
  Server server;

  boost::system::error_code run_ec;
  server.Run(endpoint_query, run_ec);

  ASSERT_EQ(0, run_ec.value());

  server.Stop();
}

TEST(SSFServerTest, listeningLocalhostInterface) {
  boost::log::core::get()->set_filter(boost::log::trivial::severity >=
                                      boost::log::trivial::info);

  using Server =
      ssf::SSFServer<ssf::network::Protocol, ssf::TransportProtocolPolicy>;

  ssf::Config ssf_config;

  auto endpoint_query =
      ssf::network::GenerateServerQuery("127.0.0.1", "8000", ssf_config);
  Server server;

  boost::system::error_code run_ec;
  server.Run(endpoint_query, run_ec);

  ASSERT_EQ(0, run_ec.value());

  server.Stop();
}

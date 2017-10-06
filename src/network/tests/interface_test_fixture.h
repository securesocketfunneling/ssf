#ifndef SSF_TESTS_INTERFACE_TEST_FIXTURE_H_
#define SSF_TESTS_INTERFACE_TEST_FIXTURE_H_

#include <gtest/gtest.h>

#include <map>

#include <boost/asio/io_service.hpp>

#include "ssf/layer/data_link/basic_circuit_protocol.h"
#include "ssf/layer/data_link/simple_circuit_policy.h"
#include "ssf/layer/data_link/circuit_helpers.h"

#include "ssf/layer/interface_layer/basic_interface_protocol.h"
#include "ssf/layer/interface_layer/basic_interface.h"

#include "ssf/layer/parameters.h"

#include "ssf/layer/physical/tcp.h"
#include "ssf/layer/physical/tlsotcp.h"

#include "ssf/log/log.h"

#include "tests/virtual_network_helpers.h"
#include "tests/circuit_test_fixture.h"

class InterfaceTestFixture : public CircuitTestFixture {
 protected:
  using SimpleLinkProtocol = ssf::layer::physical::TCPPhysicalLayer;
  using SimpleTLSLinkProtocol =
      ssf::layer::physical::TLSboTCPPhysicalLayer;
  // virtual_network::physical_layer::TLSoTCPPhysicalLayer;

  using CircuitLinkProtocol =
      ssf::layer::data_link::basic_CircuitProtocol<
          PhysicalProtocol, ssf::layer::data_link::CircuitPolicy>;
  using CircuitTLSLinkProtocol =
      ssf::layer::data_link::basic_CircuitProtocol<
          TLSPhysicalProtocol, ssf::layer::data_link::CircuitPolicy>;

  using InterfaceProtocol =
      ssf::layer::interface_layer::basic_InterfaceProtocol;

  template <class NextLayer>
  using Interface =
      ssf::layer::interface_layer::basic_Interface<InterfaceProtocol,
                                                        NextLayer>;

  using SimpleInterface = Interface<SimpleLinkProtocol>;
  using SimpleTLSInterface = Interface<SimpleTLSLinkProtocol>;
  using CircuitInterface = Interface<CircuitLinkProtocol>;
  using CircuitTLSInterface = Interface<CircuitTLSLinkProtocol>;

 protected:
  InterfaceTestFixture()
      : CircuitTestFixture(),
        simple_interfaces_(),
        simple_tls_interfaces_(),
        circuit_interfaces_(),
        circuit_tls_interfaces_(),
        promises_() {}

  virtual ~InterfaceTestFixture() {}

  virtual void SetUp() {
    CircuitTestFixture::SetUp();

    promises_.insert(std::make_pair("simple_lo1", std::promise<bool>()));
    promises_.insert(std::make_pair("simple_lo2", std::promise<bool>()));
    promises_.insert(std::make_pair("simple_tls_lo1", std::promise<bool>()));
    promises_.insert(std::make_pair("simple_tls_lo2", std::promise<bool>()));
    promises_.insert(std::make_pair("circuit_lo1", std::promise<bool>()));
    promises_.insert(std::make_pair("circuit_lo2", std::promise<bool>()));
    promises_.insert(std::make_pair("circuit_tls_lo1", std::promise<bool>()));
    promises_.insert(std::make_pair("circuit_tls_lo2", std::promise<bool>()));

    boost::system::error_code ec;

    //----------------------- simple interfaces -----------------------------//
    SimpleLinkProtocol::resolver simple_resolver(this->io_service_);

    ssf::layer::LayerParameters simple1_acceptor_physical_parameters;
    simple1_acceptor_physical_parameters["port"] = "6000";
    ssf::layer::ParameterStack simple1_parameters;
    simple1_parameters.push_back(simple1_acceptor_physical_parameters);
    auto simple1_local_endpoint_it =
        simple_resolver.resolve(simple1_parameters, ec);
    SimpleLinkProtocol::endpoint simple1_local_endpoint(
        *simple1_local_endpoint_it);
    auto it_simple1 =
        simple_interfaces_.emplace("simple_lo1", this->io_service_);

    it_simple1.first->second.async_accept(
        "simple_lo1", simple1_local_endpoint,
        std::bind(&InterfaceTestFixture::InterfaceOnline, this, "simple_lo1",
                    std::placeholders::_1));

    ssf::layer::LayerParameters simple2_connect_physical_parameters;
    simple2_connect_physical_parameters["addr"] = "127.0.0.1";
    simple2_connect_physical_parameters["port"] = "6000";
    ssf::layer::ParameterStack simple2_parameters;
    simple2_parameters.push_back(simple2_connect_physical_parameters);
    auto simple2_remote_endpoint_it =
        simple_resolver.resolve(simple2_parameters, ec);
    SimpleLinkProtocol::endpoint simple2_remote_endpoint(
        *simple2_remote_endpoint_it);

    auto it_simple2 =
        simple_interfaces_.emplace("simple_lo2", this->io_service_);
    it_simple2.first->second.async_connect(
        "simple_lo2", simple2_remote_endpoint,
        std::bind(&InterfaceTestFixture::InterfaceOnline, this, "simple_lo2",
                  std::placeholders::_1));

    //---------------------- simple TLS interfaces --------------------------//
    SimpleTLSLinkProtocol::resolver simple_tls_resolver(this->io_service_);

    boost::system::error_code tls1_ec;
    ssf::layer::LayerParameters simple_tls1_acceptor_physical_parameters;
    simple_tls1_acceptor_physical_parameters["port"] = "5000";
    ssf::layer::ParameterStack simple_tls1_parameters;
    simple_tls1_parameters.push_back(
        tests::virtual_network_helpers::GetServerTLSParametersAsBuffer());
    simple_tls1_parameters.push_back(simple_tls1_acceptor_physical_parameters);
    auto simple_tls1_local_endpoint_it =
        simple_tls_resolver.resolve(simple_tls1_parameters, tls1_ec);
    if (tls1_ec) {
      SSF_LOG(kLogError) << "Fail resolving interface tls1 endpoint : "
                         << tls1_ec.message();
      promises_["simple_tls_lo1"].set_value(false);
    } else {
      SimpleTLSLinkProtocol::endpoint simple_tls1_local_endpoint(
          *simple_tls1_local_endpoint_it);
      auto it_simple_tls1 =
          simple_tls_interfaces_.emplace("simple_tls_lo1", this->io_service_);

      it_simple_tls1.first->second.async_accept(
          "simple_tls_lo1", simple_tls1_local_endpoint,
          std::bind(&InterfaceTestFixture::InterfaceOnline, this,
                    "simple_tls_lo1", std::placeholders::_1));
    }

    boost::system::error_code tls2_ec;
    ssf::layer::LayerParameters simple_tls2_client_physical_parameters;
    simple_tls2_client_physical_parameters["addr"] = "127.0.0.1";
    simple_tls2_client_physical_parameters["port"] = "5000";
    ssf::layer::ParameterStack simple_tls2_parameters;
    simple_tls2_parameters.push_back(
        tests::virtual_network_helpers::GetClientTLSParametersAsBuffer());
    simple_tls2_parameters.push_back(simple_tls2_client_physical_parameters);
    auto simple_tls2_remote_endpoint_it =
        simple_tls_resolver.resolve(simple_tls2_parameters, tls2_ec);
    if (tls2_ec) {
      SSF_LOG(kLogError) << "Fail resolving interface tls2 endpoint : "
                         << tls2_ec.message();
      promises_["simple_tls_lo2"].set_value(false);
    } else {
      SimpleTLSLinkProtocol::endpoint simple_tls2_remote_endpoint(
          *simple_tls2_remote_endpoint_it);
      auto it_simple_tls2 =
          simple_tls_interfaces_.emplace("simple_tls_lo2", this->io_service_);

      it_simple_tls2.first->second.async_connect(
          "simple_tls_lo2", simple_tls2_remote_endpoint,
          std::bind(&InterfaceTestFixture::InterfaceOnline, this,
                    "simple_tls_lo2", std::placeholders::_1));
    }

    //------------------------- circuit interfaces --------------------------//
    CircuitLinkProtocol::resolver circuit_resolver(this->io_service_);

    ssf::layer::LayerParameters circuit1_acceptor_physical_parameters;
    circuit1_acceptor_physical_parameters["port"] = "7002";
    ssf::layer::ParameterStack pre_circuit1_parameters;
    ssf::layer::ParameterStack circuit1_acceptor_default_parameters = {{}, {}};

    pre_circuit1_parameters.push_back(circuit1_acceptor_physical_parameters);
    ssf::layer::ParameterStack circuit1_parameters(
        ssf::layer::data_link::make_acceptor_parameter_stack(
            "server", circuit1_acceptor_default_parameters,
            pre_circuit1_parameters));
    auto circuit1_local_endpoint_it =
        circuit_resolver.resolve(circuit1_parameters, ec);
    CircuitLinkProtocol::endpoint circuit1_local_endpoint(
        *circuit1_local_endpoint_it);
    auto it_circuit1 =
        circuit_interfaces_.emplace("circuit_lo1", this->io_service_);

    it_circuit1.first->second.async_accept(
        "circuit_lo1", circuit1_local_endpoint,
        std::bind(&InterfaceTestFixture::InterfaceOnline, this, "circuit_lo1",
                  std::placeholders::_1));

    ssf::layer::LayerParameters circuit2_client_physical_parameters;
    circuit2_client_physical_parameters["addr"] = "127.0.0.1";
    circuit2_client_physical_parameters["port"] = "7002";
    ssf::layer::data_link::NodeParameterList nodes(
        this->GetClientNodes());
    nodes.PushBackNode();
    nodes.AddTopLayerToBackNode(circuit2_client_physical_parameters);

    ssf::layer::ParameterStack circuit2_parameters(
        ssf::layer::data_link::
            make_client_full_circuit_parameter_stack("server", nodes));
    auto circuit2_local_endpoint_it =
        circuit_resolver.resolve(circuit2_parameters, ec);
    CircuitLinkProtocol::endpoint circuit2_local_endpoint(
        *circuit2_local_endpoint_it);
    auto it_circuit2 =
        circuit_interfaces_.emplace("circuit_lo2", this->io_service_);

    it_circuit2.first->second.async_connect(
        "circuit_lo2", circuit2_local_endpoint,
        std::bind(&InterfaceTestFixture::InterfaceOnline, this, "circuit_lo2",
                  std::placeholders::_1));

    //----------------------- circuit TLS interfaces ------------------------//
    CircuitTLSLinkProtocol::resolver circuit_tls_resolver(this->io_service_);

    boost::system::error_code circuit_tls1_ec;
    ssf::layer::LayerParameters circuit_tls1_acceptor_physical_parameters;
    circuit_tls1_acceptor_physical_parameters["port"] = "8002";
    ssf::layer::ParameterStack pre_circuit_tls1_parameters;
    pre_circuit_tls1_parameters.push_back(
        tests::virtual_network_helpers::GetServerTLSParametersAsBuffer());
    pre_circuit_tls1_parameters.push_back(
        circuit_tls1_acceptor_physical_parameters);
    ssf::layer::ParameterStack circuit_tls1_acceptor_default_parameters = {
        {}, {}, {}};
    ssf::layer::ParameterStack circuit_tls1_parameters(
        ssf::layer::data_link::make_acceptor_parameter_stack(
            "server_tls", circuit_tls1_acceptor_default_parameters,
            pre_circuit_tls1_parameters));
    auto circuit_tls1_local_endpoint_it =
        circuit_tls_resolver.resolve(circuit_tls1_parameters, circuit_tls1_ec);

    if (circuit_tls1_ec) {
      SSF_LOG(kLogError) << "Fail resolving interface circuit tls1 endpoint : "
                         << circuit_tls1_ec.message();
      promises_["circuit_tls_lo1"].set_value(false);
    } else {
      CircuitTLSLinkProtocol::endpoint circuit_tls1_local_endpoint(
          *circuit_tls1_local_endpoint_it);
      auto it_circuit_tls1 =
          circuit_tls_interfaces_.emplace("circuit_tls_lo1", this->io_service_);

      it_circuit_tls1.first->second.async_accept(
          "circuit_tls_lo1", circuit_tls1_local_endpoint,
          std::bind(&InterfaceTestFixture::InterfaceOnline, this,
                      "circuit_tls_lo1", std::placeholders::_1));
    }

    boost::system::error_code circuit_tls2_ec;
    ssf::layer::LayerParameters circuit_tls2_client_physical_parameters;
    circuit_tls2_client_physical_parameters["addr"] = "127.0.0.1";
    circuit_tls2_client_physical_parameters["port"] = "8002";
    ssf::layer::data_link::NodeParameterList nodes_tls(
        this->GetClientTLSNodes());
    nodes_tls.PushBackNode();
    nodes_tls.AddTopLayerToBackNode(circuit_tls2_client_physical_parameters);
    nodes_tls.AddTopLayerToBackNode(
        tests::virtual_network_helpers::GetClientTLSParametersAsBuffer());

    ssf::layer::ParameterStack circuit_tls2_parameters(
        ssf::layer::data_link::
            make_client_full_circuit_parameter_stack("server_tls", nodes_tls));
    auto circuit_tls2_local_endpoint_it =
        circuit_tls_resolver.resolve(circuit_tls2_parameters, circuit_tls2_ec);

    if (circuit_tls2_ec) {
      SSF_LOG(kLogError) << "Fail resolving interface circuit tls2 endpoint : "
                         << circuit_tls2_ec.message();
      promises_["circuit_tls_lo2"].set_value(false);
    } else {
      CircuitTLSLinkProtocol::endpoint circuit_tls2_local_endpoint(
          *circuit_tls2_local_endpoint_it);
      auto it_circuit_tls2 =
          circuit_tls_interfaces_.emplace("circuit_tls_lo2", this->io_service_);

      it_circuit_tls2.first->second.async_connect(
          "circuit_tls_lo2", circuit_tls2_local_endpoint,
          std::bind(&InterfaceTestFixture::InterfaceOnline, this,
                      "circuit_tls_lo2", std::placeholders::_1));
    }

    Wait();
  }

  virtual void TearDown() {
    boost::system::error_code ec;

    for (auto& interface_pair : simple_interfaces_) {
      interface_pair.second.close(ec);
    }
    simple_interfaces_.clear();

    for (auto& interface_pair : simple_tls_interfaces_) {
      interface_pair.second.close(ec);
    }
    simple_tls_interfaces_.clear();

    for (auto& interface_pair : circuit_interfaces_) {
      interface_pair.second.close(ec);
    }
    circuit_interfaces_.clear();

    for (auto& interface_pair : circuit_tls_interfaces_) {
      interface_pair.second.close(ec);
    }
    circuit_tls_interfaces_.clear();

    CircuitTestFixture::TearDown();
  }

  void InterfaceOnline(const std::string& interface_id,
                       const boost::system::error_code& ec) {
    if (!ec) {
      SSF_LOG(kLogInfo) << "Interface up : " << interface_id;
    }
    promises_[interface_id].set_value(!ec);
  }

 private:
  bool Wait() {
    bool result = true;

    for (auto& promise : promises_) {
      result &= promise.second.get_future().get();
    }

    return result;
  }

 protected:
  std::map<std::string, SimpleInterface> simple_interfaces_;
  std::map<std::string, SimpleTLSInterface> simple_tls_interfaces_;
  std::map<std::string, CircuitInterface> circuit_interfaces_;
  std::map<std::string, CircuitTLSInterface> circuit_tls_interfaces_;

  std::map<std::string, std::promise<bool>> promises_;
};

#endif  // SSF_TESTS_INTERFACE_TEST_FIXTURE_H_

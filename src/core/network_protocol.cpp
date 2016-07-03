#include "core/network_protocol.h"

namespace ssf {
namespace network {

NetworkProtocol::Query NetworkProtocol::GenerateClientQuery(
    const std::string& remote_addr, const std::string& remote_port,
    const ssf::config::Config& ssf_config,
    const ssf::circuit::NodeList& circuit_nodes) {
#ifdef TLS_OVER_TCP_LINK
  return GenerateClientTLSQuery(remote_addr, remote_port, ssf_config,
                                circuit_nodes);
#elif TCP_ONLY_LINK
  return GenerateClientTCPQuery(remote_addr, remote_port, circuit_nodes);
#endif
}

NetworkProtocol::Query NetworkProtocol::GenerateServerQuery(
    const std::string& remote_addr, const std::string& remote_port,
    const ssf::config::Config& ssf_config) {
#ifdef TLS_OVER_TCP_LINK
  return GenerateServerTLSQuery(remote_addr, remote_port, ssf_config);
#elif TCP_ONLY_LINK
  return NetworkProtocol::GenerateServerTCPQuery(remote_addr, remote_port);
#endif
}

NetworkProtocol::Query NetworkProtocol::GenerateClientTCPQuery(
    const std::string& remote_addr, const std::string& remote_port,
    const ssf::config::Config& ssf_config,
    const ssf::circuit::NodeList& circuit_nodes) {
  ssf::layer::LayerParameters proxy_param_layer =
      ProxyConfigToLayerParameters(ssf_config);

  ssf::layer::LayerParameters default_param_layer = {{"default", "true"}};

  ssf::layer::data_link::NodeParameterList nodes;

  nodes.PushBackNode();
  nodes.AddTopLayerToBackNode({{"addr", remote_addr}, {"port", remote_port}});
  nodes.AddTopLayerToBackNode(proxy_param_layer);

  for (auto& circuit_node : circuit_nodes) {
    nodes.PushBackNode();
    nodes.AddTopLayerToBackNode(
        {{"addr", circuit_node.addr()}, {"port", circuit_node.port()}});
    nodes.AddTopLayerToBackNode(default_param_layer);
  }

  return ssf::layer::data_link::make_client_full_circuit_parameter_stack(
      "client", nodes);
}

NetworkProtocol::Query NetworkProtocol::GenerateClientTLSQuery(
    const std::string& remote_addr, const std::string& remote_port,
    const ssf::config::Config& ssf_config,
    const ssf::circuit::NodeList& circuit_nodes) {
  ssf::layer::LayerParameters tls_param_layer =
      TlsConfigToLayerParameters(ssf_config);

  ssf::layer::LayerParameters proxy_param_layer =
      ProxyConfigToLayerParameters(ssf_config);

  ssf::layer::LayerParameters default_param_layer = {{"default", "true"}};

  ssf::layer::data_link::NodeParameterList nodes;

  nodes.PushBackNode();
  nodes.AddTopLayerToBackNode({{"addr", remote_addr}, {"port", remote_port}});
  nodes.AddTopLayerToBackNode(proxy_param_layer);
  nodes.AddTopLayerToBackNode(tls_param_layer);

  for (auto& circuit_node : circuit_nodes) {
    nodes.PushBackNode();
    nodes.AddTopLayerToBackNode(
        {{"addr", circuit_node.addr()}, {"port", circuit_node.port()}});
    nodes.AddTopLayerToBackNode(default_param_layer);
    nodes.AddTopLayerToBackNode(default_param_layer);
  }

  Query query = ssf::layer::data_link::make_client_full_circuit_parameter_stack(
      "client", nodes);

  query.push_front(tls_param_layer);

  return query;
}

NetworkProtocol::Query NetworkProtocol::GenerateServerTCPQuery(
    const std::string& remote_addr, const std::string& remote_port,
    const ssf::config::Config& ssf_config) {
  NetworkProtocol::Query query;
  ssf::layer::LayerParameters physical_parameters;
  physical_parameters["port"] = remote_port;
  if (!remote_addr.empty()) {
    physical_parameters["addr"] = remote_addr;
  }

  ssf::layer::LayerParameters proxy_param_layer =
      ProxyConfigToLayerParameters(ssf_config);

  ssf::layer::ParameterStack layer_parameters;
  layer_parameters.push_front(physical_parameters);

  ssf::layer::ParameterStack default_parameters = {proxy_param_layer, {}};

  return ssf::layer::data_link::make_forwarding_acceptor_parameter_stack(
      "server", default_parameters, layer_parameters);
}

NetworkProtocol::Query NetworkProtocol::GenerateServerTLSQuery(
    const std::string& remote_addr, const std::string& remote_port,
    const ssf::config::Config& ssf_config) {
  ssf::layer::LayerParameters tls_param_layer =
      TlsConfigToLayerParameters(ssf_config);

  ssf::layer::LayerParameters physical_parameters;
  physical_parameters["port"] = remote_port;
  if (!remote_addr.empty()) {
    physical_parameters["addr"] = remote_addr;
  }

  ssf::layer::LayerParameters proxy_param_layer =
      ProxyConfigToLayerParameters(ssf_config);

  ssf::layer::ParameterStack layer_parameters;
  layer_parameters.push_front(physical_parameters);
  layer_parameters.push_front(proxy_param_layer);
  layer_parameters.push_front(tls_param_layer);

  ssf::layer::ParameterStack default_parameters = {
      {}, tls_param_layer, proxy_param_layer, {}};

  Query query = ssf::layer::data_link::make_forwarding_acceptor_parameter_stack(
      "server", default_parameters, layer_parameters);

  query.push_front(tls_param_layer);

  return query;
}

ssf::layer::LayerParameters NetworkProtocol::TlsConfigToLayerParameters(
    const ssf::config::Config& ssf_config) {
  return {{"ca_src", "file"},
          {"crt_src", "file"},
          {"key_src", "file"},
          {"dhparam_src", "file"},
          {"ca_file", ssf_config.tls().ca_cert_path()},
          {"crt_file", ssf_config.tls().cert_path()},
          {"key_file", ssf_config.tls().key_path()},
          {"password", ssf_config.tls().key_password()},
          {"dhparam_file", ssf_config.tls().dh_path()},
          {"set_cipher_suit", ssf_config.tls().cipher_alg()}};
}

ssf::layer::LayerParameters NetworkProtocol::ProxyConfigToLayerParameters(
    const ssf::config::Config& ssf_config) {
  return {{"http_addr", ssf_config.proxy().http_addr()},
          {"http_port", ssf_config.proxy().http_port()},
          {"http_username", ssf_config.proxy().http_username()},
          {"http_password", ssf_config.proxy().http_password()}};
}

}  // network
}  // ssf
#include <ssf/utils/enum.h>

#include "common/config/config.h"

#include "core/network_protocol.h"

namespace ssf {
namespace network {

NetworkProtocol::Query NetworkProtocol::GenerateClientQuery(
    const std::string& remote_addr, const std::string& remote_port,
    const ssf::config::Config& ssf_config,
    const ssf::config::NodeList& circuit_nodes) {
#ifdef TLS_OVER_TCP_LINK
  return GenerateClientTLSQuery(remote_addr, remote_port, ssf_config,
                                circuit_nodes);
#elif TCP_ONLY_LINK
  return GenerateClientTCPQuery(remote_addr, remote_port, ssf_config,
                                circuit_nodes);
#endif
}

NetworkProtocol::Query NetworkProtocol::GenerateServerQuery(
    const std::string& remote_addr, const std::string& remote_port,
    const ssf::config::Config& ssf_config) {
#ifdef TLS_OVER_TCP_LINK
  return GenerateServerTLSQuery(remote_addr, remote_port, ssf_config);
#elif TCP_ONLY_LINK
  return GenerateServerTCPQuery(remote_addr, remote_port, ssf_config);
#endif
}

NetworkProtocol::Query NetworkProtocol::GenerateClientTCPQuery(
    const std::string& remote_addr, const std::string& remote_port,
    const ssf::config::Config& ssf_config,
    const ssf::config::NodeList& circuit_nodes) {
  ssf::layer::LayerParameters proxy_param_layer =
      ProxyConfigToLayerParameters(ssf_config, false);

  ssf::layer::LayerParameters default_param_layer = {{"default", "true"}};

  ssf::layer::data_link::NodeParameterList nodes;

  nodes.PushBackNode();
  nodes.AddTopLayerToBackNode({{"addr", remote_addr}, {"port", remote_port}});
  nodes.AddTopLayerToBackNode(proxy_param_layer);

  for (const auto& circuit_node : circuit_nodes) {
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
    const ssf::config::NodeList& circuit_nodes) {
  ssf::layer::LayerParameters tls_param_layer =
      TlsConfigToLayerParameters(ssf_config);

  ssf::layer::LayerParameters proxy_param_layer =
      ProxyConfigToLayerParameters(ssf_config, false);

  ssf::layer::LayerParameters default_param_layer = {{"default", "true"}};

  ssf::layer::data_link::NodeParameterList nodes;

  nodes.PushBackNode();
  nodes.AddTopLayerToBackNode({{"addr", remote_addr}, {"port", remote_port}});
  nodes.AddTopLayerToBackNode(proxy_param_layer);
  nodes.AddTopLayerToBackNode(tls_param_layer);

  for (const auto& circuit_node : circuit_nodes) {
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
      ProxyConfigToLayerParameters(ssf_config, true);

  ssf::layer::LayerParameters default_proxy_param_layer =
      ProxyConfigToLayerParameters(ssf_config, false);

  ssf::layer::ParameterStack layer_parameters;
  layer_parameters.push_front(physical_parameters);
  layer_parameters.push_front(proxy_param_layer);

  ssf::layer::ParameterStack default_parameters = {default_proxy_param_layer,
                                                   {}};

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
      ProxyConfigToLayerParameters(ssf_config, true);

  ssf::layer::LayerParameters default_proxy_param_layer =
      ProxyConfigToLayerParameters(ssf_config, false);

  ssf::layer::ParameterStack layer_parameters;
  layer_parameters.push_front(physical_parameters);
  layer_parameters.push_front(proxy_param_layer);
  layer_parameters.push_front(tls_param_layer);

  ssf::layer::ParameterStack default_parameters = {
      {}, tls_param_layer, default_proxy_param_layer, {}};

  Query query = ssf::layer::data_link::make_forwarding_acceptor_parameter_stack(
      "server", default_parameters, layer_parameters);

  query.push_front(tls_param_layer);

  return query;
}

ssf::layer::LayerParameters NetworkProtocol::TlsConfigToLayerParameters(
    const ssf::config::Config& ssf_config) {
  return {{ssf_config.tls().ca_cert().IsBuffer() ? "ca_buffer" : "ca_file",
           ssf_config.tls().ca_cert().value()},
          {ssf_config.tls().cert().IsBuffer() ? "crt_buffer" : "crt_file",
           ssf_config.tls().cert().value()},
          {ssf_config.tls().key().IsBuffer() ? "key_buffer" : "key_file",
           ssf_config.tls().key().value()},
          {"key_password", ssf_config.tls().key_password()},
          {ssf_config.tls().dh().IsBuffer() ? "dhparam_buffer" : "dhparam_file",
           ssf_config.tls().dh().value()},
          {"cipher_suit", ssf_config.tls().cipher_alg()}};
}

ssf::layer::LayerParameters NetworkProtocol::ProxyConfigToLayerParameters(
    const ssf::config::Config& ssf_config, bool acceptor_endpoint) {
  return {{"acceptor_endpoint", acceptor_endpoint ? "true" : "false"},
          {"http_host", ssf_config.http_proxy().host()},
          {"http_port", ssf_config.http_proxy().port()},
          {"http_username", ssf_config.http_proxy().username()},
          {"http_domain", ssf_config.http_proxy().domain()},
          {"http_password", ssf_config.http_proxy().password()},
          {"http_user_agent", ssf_config.http_proxy().user_agent()},
          {"http_reuse_ntlm",
           ssf_config.http_proxy().reuse_ntlm() ? "true" : "false"},
          {"http_reuse_kerb",
           ssf_config.http_proxy().reuse_kerb() ? "true" : "false"},
          {"socks_version",
           std::to_string(ToIntegral(ssf_config.socks_proxy().version()))},
          {"socks_host", ssf_config.socks_proxy().host()},
          {"socks_port", ssf_config.socks_proxy().port()}};
}

}  // network
}  // ssf
#include "core/network_protocol.h"

#include "core/parser/bounce_parser.h"

namespace ssf {
namespace network {

Query GenerateClientQuery(const std::string& remote_addr,
                          const std::string& remote_port,
                          const ssf::Config& ssf_config,
                          const CircuitBouncers& bouncers) {
#ifdef TLS_OVER_TCP_LINK
  return GenerateClientTLSQuery(remote_addr, remote_port,
                                              ssf_config, bouncers);
#elif TCP_ONLY_LINK
  return GenerateClientTCPQuery(remote_addr, remote_port,
                                              bouncers);
#endif
}

Query GenerateServerQuery(const std::string& remote_port,
                          const ssf::Config& ssf_config) {
#ifdef TLS_OVER_TCP_LINK
  return GenerateServerTLSQuery(remote_port, ssf_config);
#elif TCP_ONLY_LINK
  return GenerateServerTCPQuery(remote_port, ssf_config);
#endif
}

Query GenerateClientTCPQuery(const std::string& remote_addr,
                             const std::string& remote_port,
                             const CircuitBouncers& bouncers) {
  ssf::layer::data_link::NodeParameterList nodes;

  nodes.PushBackNode();
  nodes.AddTopLayerToBackNode({{"addr", remote_addr}, {"port", remote_port}});

  for (auto& bouncer : bouncers) {
    auto addr = ssf::parser::BounceParser::GetRemoteAddress(bouncer);
    auto port = ssf::parser::BounceParser::GetRemotePort(bouncer);
    nodes.PushBackNode();
    nodes.AddTopLayerToBackNode({{"addr", addr}, {"port", port}});
  }

  return ssf::layer::data_link::make_client_full_circuit_parameter_stack(
      "client", nodes);
}

Query GenerateClientTLSQuery(const std::string& remote_addr,
                             const std::string& remote_port,
                             const ssf::Config& ssf_config,
                             const CircuitBouncers& bouncers) {
  ssf::layer::LayerParameters tls_param_layer = {
      {"ca_src", "file"},
      {"crt_src", "file"},
      {"key_src", "file"},
      {"dhparam_src", "file"},
      {"ca_file", ssf_config.tls.ca_cert_path},
      {"crt_file", ssf_config.tls.cert_path},
      {"key_file", ssf_config.tls.key_path},
      {"dhparam_file", ssf_config.tls.dh_path},
      {"set_cipher_suit", ssf_config.tls.cipher_alg}};

  ssf::layer::LayerParameters tls_default_param_layer = {
      {}, {"default", "true"}, {}};

  ssf::layer::data_link::NodeParameterList nodes;

  nodes.PushBackNode();
  nodes.AddTopLayerToBackNode({{"addr", remote_addr}, {"port", remote_port}});
  nodes.AddTopLayerToBackNode(tls_param_layer);

  for (auto& bouncer : bouncers) {
    auto addr = ssf::parser::BounceParser::GetRemoteAddress(bouncer);
    auto port = ssf::parser::BounceParser::GetRemotePort(bouncer);
    nodes.PushBackNode();
    nodes.AddTopLayerToBackNode({{"addr", addr}, {"port", port}});
    nodes.AddTopLayerToBackNode(tls_default_param_layer);
  }

  Query query =
      ssf::layer::data_link::make_client_full_circuit_parameter_stack("client",
                                                                      nodes);

  query.push_front(tls_param_layer);

  return query;
}

Query GenerateServerTCPQuery(const std::string& remote_port) {
  Query query;
  ssf::layer::LayerParameters physical_parameters;
  physical_parameters["port"] = remote_port;

  ssf::layer::ParameterStack layer_parameters;
  layer_parameters.push_front(physical_parameters);

  return ssf::layer::data_link::make_forwarding_acceptor_parameter_stack(
      "server", {}, layer_parameters);
}

Query GenerateServerTLSQuery(const std::string& remote_port,
                             const ssf::Config& ssf_config) {
  ssf::layer::LayerParameters tls_param_layer = {
      {"ca_src", "file"},
      {"crt_src", "file"},
      {"key_src", "file"},
      {"dhparam_src", "file"},
      {"ca_file", ssf_config.tls.ca_cert_path},
      {"crt_file", ssf_config.tls.cert_path},
      {"key_file", ssf_config.tls.key_path},
      {"dhparam_file", ssf_config.tls.dh_path},
      {"set_cipher_suit", ssf_config.tls.cipher_alg}};

  ssf::layer::LayerParameters physical_parameters;
  physical_parameters["port"] = remote_port;

  ssf::layer::ParameterStack layer_parameters;
  layer_parameters.push_front(physical_parameters);
  layer_parameters.push_front(tls_param_layer);

  ssf::layer::ParameterStack default_parameters = {{}, tls_param_layer, {}};

  Query query =
      ssf::layer::data_link::make_forwarding_acceptor_parameter_stack(
          "server", default_parameters, layer_parameters);

  query.push_front(tls_param_layer);

  return query;
}

}  // network
}  // ssf
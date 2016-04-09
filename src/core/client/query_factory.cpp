#include "core/client/query_factory.h"

#include <ssf/layer/data_link/circuit_helpers.h>

#include "core/parser/bounce_parser.h"

namespace ssf {

NetworkQuery GenerateTCPNetworkQuery(
    const std::string& remote_addr, const std::string& remote_port,
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

NetworkQuery GenerateTLSNetworkQuery(
    const std::string& remote_addr, const std::string& remote_port,
    const ssf::Config& ssf_config, const CircuitBouncers& bouncers) {
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

  return ssf::layer::data_link::make_client_full_circuit_parameter_stack(
      "client", nodes);
}

}
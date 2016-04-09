#include "core/server/query_factory.h"

#include <ssf/layer/data_link/circuit_helpers.h>

namespace ssf {

NetworkQuery GenerateServerTCPNetworkQuery(const std::string& remote_port) {
  NetworkQuery query;
  ssf::layer::LayerParameters physical_parameters;
  physical_parameters["port"] = remote_port;

  ssf::layer::ParameterStack layer_parameters;
  layer_parameters.push_front(physical_parameters);

  return ssf::layer::data_link::make_forwarding_acceptor_parameter_stack(
      "server", {}, layer_parameters);
}

NetworkQuery GenerateServerTLSNetworkQuery(const std::string& remote_port,
                                           const ssf::Config& ssf_config){
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

  NetworkQuery query;
  ssf::layer::LayerParameters physical_parameters;
  physical_parameters["port"] = remote_port;

  ssf::layer::ParameterStack layer_parameters;
  layer_parameters.push_front(physical_parameters);
  layer_parameters.push_front(tls_param_layer);

  ssf::layer::ParameterStack default_parameters = {{}, tls_param_layer, {}};

  return ssf::layer::data_link::make_forwarding_acceptor_parameter_stack(
      "server", default_parameters, layer_parameters);
}

}
#ifndef SSF_LAYER_DATA_LINK_CIRCUIT_PROTOCOL_H_
#define SSF_LAYER_DATA_LINK_CIRCUIT_PROTOCOL_H_

#include <cstdint>

#include <map>
#include <thread>

#include <boost/asio/io_service.hpp>
#include <boost/asio/basic_socket_acceptor.hpp>
#include <boost/asio/basic_stream_socket.hpp>

#include <boost/system/error_code.hpp>

#include "ssf/layer/basic_resolver.h"
#include "ssf/layer/basic_endpoint.h"

#include "ssf/layer/data_link/basic_circuit_acceptor_service.h"
#include "ssf/layer/data_link/basic_circuit_socket_service.h"
#include "ssf/layer/data_link/circuit_helpers.h"
#include "ssf/layer/data_link/circuit_endpoint_context.h"

#include "ssf/layer/parameters.h"
#include "ssf/layer/protocol_attributes.h"

namespace ssf {
namespace layer {
namespace data_link {

template <class NextLayer, template <class> class CircuitPolicy>
class basic_CircuitProtocol {
 public:
  enum {
    id = 5,
    overhead = 0,
    facilities = ssf::layer::facilities::stream,
    mtu = NextLayer::mtu - overhead
  };
  enum { endpoint_stack_size = 1 + NextLayer::endpoint_stack_size };

  static const char* NAME;

  typedef CircuitPolicy<basic_CircuitProtocol> circuit_policy;
  typedef NextLayer next_layer_protocol;
  typedef int socket_context;
  typedef int acceptor_context;
  typedef CircuitEndpointContext endpoint_context_type;
  using next_endpoint_type = typename next_layer_protocol::endpoint;

  typedef basic_VirtualLink_endpoint<basic_CircuitProtocol> endpoint;
  typedef basic_VirtualLink_resolver<basic_CircuitProtocol> resolver;
  typedef boost::asio::basic_stream_socket<
      basic_CircuitProtocol, basic_CircuitSocket_service<basic_CircuitProtocol>>
      socket;
  typedef boost::asio::basic_socket_acceptor<
      basic_CircuitProtocol,
      basic_CircuitAcceptor_service<basic_CircuitProtocol>> acceptor;

 private:
  using query = typename resolver::query;

 public:
  static std::string get_name() {
    std::string name(NAME);
    name += "_" + next_layer_protocol::get_name();
    return name;
  }

  static endpoint make_endpoint(boost::asio::io_service& io_service,
                                typename query::const_iterator parameters_it,
                                uint32_t, boost::system::error_code& ec) {
    auto context = detail::make_circuit_context(io_service, *parameters_it);
    auto next_layer_endpoint =
        next_layer_protocol::make_endpoint(io_service, ++parameters_it, id, ec);

    return endpoint(context, next_layer_endpoint);
  }

  static void add_params_from_property_tree(
      query* p_query, const boost::property_tree::ptree& property_tree,
      bool connect, boost::system::error_code& ec) {
    auto layer_name = property_tree.get_child_optional("layer");
    if (!layer_name || layer_name.get().data() != NAME) {
      ec.assign(ssf::error::invalid_argument, ssf::error::get_ssf_category());
      return;
    }

    auto layer_parameters = property_tree.get_child_optional("parameters");
    if (!layer_parameters) {
      ec.assign(ssf::error::missing_config_parameters,
                ssf::error::get_ssf_category());
      return;
    }

    auto sublayer = property_tree.get_child_optional("sublayer");
    if (!sublayer) {
      ec.assign(ssf::error::missing_config_parameters,
                ssf::error::get_ssf_category());
      return;
    }

    bool forward = false;
    auto given_forward = (*layer_parameters).get_child_optional("forward");
    forward = given_forward && given_forward.get().get_value<bool>();

    ParameterStack next_layer_parameters;
    next_layer_protocol::add_params_from_property_tree(&next_layer_parameters,
                                                       *sublayer, connect, ec);
    if (ec) {
      return;
    }

    if (!connect) {
      // acceptor query
      auto given_local_id = (*layer_parameters).get_child_optional("local_id");
      if (!given_local_id) {
        ec.assign(ssf::error::missing_config_parameters,
                  ssf::error::get_ssf_category());
        return;
      }
      std::string local_id = given_local_id.get().data();
      ParameterStack default_params = {};
      if (!forward) {
        *p_query = make_acceptor_parameter_stack(local_id, default_params,
                                                 next_layer_parameters);
      } else {
        *p_query = make_forwarding_acceptor_parameter_stack(
            local_id, default_params, next_layer_parameters);
      }
    } else {
      // connect query
      auto given_remote_id =
          (*layer_parameters).get_child_optional("remote_id");
      if (!given_remote_id) {
        ec.assign(ssf::error::missing_config_parameters,
                  ssf::error::get_ssf_category());
        return;
      }
      std::string remote_id = given_remote_id.get().data();
      NodeParameterList nodes;

      auto given_nodes = (*layer_parameters).get_child_optional("nodes");
      if (given_nodes && given_nodes->size() > 0) {
        nodes = nodes_property_tree_to_node_list<next_layer_protocol>(
            *given_nodes, connect, ec);
      }

      if (ec) {
        return;
      }

      nodes.PushBackNode();
      auto node_stack_end_it = next_layer_parameters.rend();
      for (auto param_node_it = next_layer_parameters.rbegin();
           param_node_it != node_stack_end_it; ++param_node_it) {
        nodes.AddTopLayerToBackNode(*param_node_it);
      }

      *p_query = make_client_full_circuit_parameter_stack(remote_id, nodes);
    }
  }
};

template <class NextLayer, template <class> class CircuitPolicy>
const char* basic_CircuitProtocol<NextLayer, CircuitPolicy>::NAME = "CIRCUIT";

}  // data_link
}  // layer
}  // ssf

#endif  // SSF_LAYER_DATA_LINK_CIRCUIT_PROTOCOL_H_

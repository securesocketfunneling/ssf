#ifndef SSF_LAYER_DATA_LINK_CIRCUIT_HELPERS_H_
#define SSF_LAYER_DATA_LINK_CIRCUIT_HELPERS_H_

#include <list>
#include <sstream>
#include <string>

#include <boost/asio/io_service.hpp>

#include <boost/property_tree/ptree.hpp>

#include "ssf/utils/map_helpers.h"

#include "ssf/layer/parameters.h"

#include "ssf/layer/data_link/circuit_endpoint_context.h"

namespace ssf {
namespace layer {
namespace data_link {

namespace detail {

CircuitEndpointContext::ID get_local_id();

LayerParameters make_next_forward_node_ciruit_layer_parameters(
    const ParameterStack &next_node_full_stack);

ParameterStack make_destination_node_parameter_stack();

CircuitEndpointContext make_circuit_context(boost::asio::io_service &io_service,
                                            const LayerParameters &parameters);

}  // detail

class NodeParameterList {
 private:
  typedef std::list<ParameterStack> NodeList;

 public:
  void PushFrontNode(ParameterStack new_node_stack = ParameterStack());

  ParameterStack &FrontNode();
  const ParameterStack &FrontNode() const;
  void PopFrontNode();

  void AddTopLayerToFrontNode(LayerParameters top_layer);

  void PushBackNode(ParameterStack new_node_stack = ParameterStack());

  ParameterStack &BackNode();
  const ParameterStack &BackNode() const;
  void PopBackNode();

  void AddTopLayerToBackNode(LayerParameters top_layer);

  NodeList::iterator begin();
  NodeList::iterator end();

  NodeList::const_iterator begin() const;
  NodeList::const_iterator end() const;

 private:
  NodeList nodes_;
};

ParameterStack make_acceptor_parameter_stack(
    std::string local_id, ParameterStack default_parameters,
    ParameterStack next_layer_parameters);

ParameterStack make_forwarding_acceptor_parameter_stack(
    std::string local_id, ParameterStack default_parameters,
    ParameterStack next_layer_parameters);

ParameterStack make_client_full_circuit_parameter_stack(
    std::string remote_id, const NodeParameterList &nodes);

template <class NodeProtocol>
NodeParameterList nodes_property_tree_to_node_list(
    const boost::property_tree::ptree &nodes_tree, bool connect,
    boost::system::error_code &ec) {
  NodeParameterList nodes;

  for (auto &node : nodes_tree) {
    ParameterStack node_stack;
    NodeProtocol::add_params_from_property_tree(&node_stack, node.second,
                                                connect, ec);
    if (ec) {
      return nodes;
    }
    nodes.PushBackNode();
    auto node_stack_end_it = node_stack.rend();
    for (auto param_node_it = node_stack.rbegin();
         param_node_it != node_stack_end_it; ++param_node_it) {
      nodes.AddTopLayerToBackNode(*param_node_it);
    }
  }

  return nodes;
}

}  // data_link
}  // layer
}  // ssf

#endif  // SSF_LAYER_DATA_LINK_CIRCUIT_HELPERS_H_
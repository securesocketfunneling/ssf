#include "ssf/layer/data_link/circuit_helpers.h"

namespace ssf {
namespace layer {
namespace data_link {
namespace detail {

CircuitEndpointContext::ID get_local_id() { return "-1"; }

LayerParameters make_next_forward_node_ciruit_layer_parameters(
    const ParameterStack &next_node_full_stack) {
  LayerParameters circuit_link_parameters;
  circuit_link_parameters["forward"] = "1";
  circuit_link_parameters["circuit_id"] = "";
  circuit_link_parameters["circuit_nodes"] =
      serialize_parameter_stack(next_node_full_stack);
  circuit_link_parameters["details"] = "";

  return circuit_link_parameters;
}

ParameterStack make_destination_node_parameter_stack() {
  LayerParameters end_parameters;
  end_parameters["forward"] = "0";
  end_parameters["circuit_id"] = "";
  end_parameters["circuit_nodes"] = "";
  end_parameters["details"] = get_local_id();

  ParameterStack end_stack;
  end_stack.push_back(std::move(end_parameters));

  return end_stack;
}

CircuitEndpointContext make_circuit_context(boost::asio::io_service &io_service,
                                            const LayerParameters &parameters) {
  auto forward_str = helpers::GetField<std::string>("forward", parameters);
  bool forward = false;

  try {
    forward = !!std::stoul(forward_str);
  } catch (const std::exception &) {
    forward = false;
  }

  auto id = helpers::GetField<std::string>("circuit_id", parameters);
  auto forward_blocks =
      helpers::GetField<std::string>("circuit_nodes", parameters);
  auto details = helpers::GetField<std::string>("details", parameters);
  auto default_parameters =
      helpers::GetField<std::string>("default_parameters", parameters);

  return CircuitEndpointContext(
      {forward, id, forward_blocks, default_parameters, details});
}

}  // detail

void NodeParameterList::PushFrontNode(ParameterStack new_node_stack) {
  nodes_.push_back(std::move(new_node_stack));
}

ParameterStack &NodeParameterList::FrontNode() { return nodes_.back(); }
const ParameterStack &NodeParameterList::FrontNode() const {
  return nodes_.back();
}
void NodeParameterList::PopFrontNode() { nodes_.pop_back(); }

void NodeParameterList::AddTopLayerToFrontNode(LayerParameters top_layer) {
  nodes_.back().push_front(std::move(top_layer));
}

void NodeParameterList::PushBackNode(ParameterStack new_node_stack) {
  nodes_.push_front(std::move(new_node_stack));
}

ParameterStack &NodeParameterList::BackNode() { return nodes_.front(); }
const ParameterStack &NodeParameterList::BackNode() const {
  return nodes_.front();
}
void NodeParameterList::PopBackNode() { nodes_.pop_front(); }

void NodeParameterList::AddTopLayerToBackNode(LayerParameters top_layer) {
  nodes_.front().push_front(std::move(top_layer));
}

NodeParameterList::NodeList::iterator NodeParameterList::begin() {
  return nodes_.begin();
}
NodeParameterList::NodeList::iterator NodeParameterList::end() {
  return nodes_.end();
}

NodeParameterList::NodeList::const_iterator NodeParameterList::begin() const {
  return nodes_.begin();
}
NodeParameterList::NodeList::const_iterator NodeParameterList::end() const {
  return nodes_.end();
}

ParameterStack make_acceptor_parameter_stack(
    std::string local_id, ParameterStack default_parameters,
    ParameterStack next_layer_parameters) {
  LayerParameters circuit_link_parameters;
  circuit_link_parameters["forward"] = "0";
  circuit_link_parameters["circuit_id"] = std::move(local_id);
  circuit_link_parameters["circuit_nodes"] = "";
  circuit_link_parameters["details"] = "";
  circuit_link_parameters["default_parameters"] =
      serialize_parameter_stack(default_parameters);

  ParameterStack acceptor_parameters(std::move(next_layer_parameters));
  acceptor_parameters.push_front(std::move(circuit_link_parameters));

  return acceptor_parameters;
}

ParameterStack make_forwarding_acceptor_parameter_stack(
    std::string local_id, ParameterStack default_parameters,
    ParameterStack next_layer_parameters) {
  LayerParameters circuit_link_parameters;
  circuit_link_parameters["forward"] = "1";
  circuit_link_parameters["circuit_id"] = std::move(local_id);
  circuit_link_parameters["circuit_nodes"] = "";
  circuit_link_parameters["details"] = "";
  circuit_link_parameters["default_parameters"] =
      serialize_parameter_stack(default_parameters);

  ParameterStack acceptor_parameters(std::move(next_layer_parameters));
  acceptor_parameters.push_front(std::move(circuit_link_parameters));

  return acceptor_parameters;
}

ParameterStack make_client_full_circuit_parameter_stack(
    std::string remote_id, const NodeParameterList &nodes) {
  auto destination_node_stack = detail::make_destination_node_parameter_stack();

  auto next_node_stack = std::move(destination_node_stack);

  for (const auto &node : nodes) {
    auto current_node_partial_stack = node;
    current_node_partial_stack.push_front(
        detail::make_next_forward_node_ciruit_layer_parameters(
            next_node_stack));
    next_node_stack = std::move(current_node_partial_stack);
  }

  next_node_stack.front()["forward"] = "0";
  next_node_stack.front()["circuit_id"] = std::move(remote_id);

  return next_node_stack;
}

}  // data_link
}  // layer
}  // ssf

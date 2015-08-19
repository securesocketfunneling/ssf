#ifndef SSF_CORE_VIRTUAL_NETWORK_DATA_LINK_CIRCUIT_HELPERS_H_
#define SSF_CORE_VIRTUAL_NETWORK_DATA_LINK_CIRCUIT_HELPERS_H_

#include <list>
#include <sstream>
#include <string>

#include <boost/asio/io_service.hpp>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <boost/property_tree/ptree.hpp>

#include <boost/serialization/list.hpp>
#include <boost/serialization/map.hpp>

#include "common/utils/map_helpers.h"

#include "core/virtual_network/parameters.h"

#include "core/virtual_network/data_link_layer/circuit_endpoint_context.h"

namespace virtual_network {
namespace data_link_layer {

namespace detail {

CircuitEndpointContext::ID get_local_id() { return "-1"; }

CircuitEndpointContext::ForwardBlock make_forward_block(
    const ParameterStack &stack) {
  std::ostringstream ostrs;
  boost::archive::text_oarchive ar(ostrs);

  ar << stack;

  return ostrs.str();
}

ParameterStack make_parameter_stack(
    const CircuitEndpointContext::ForwardBlock &forward_block) {
  try {
    std::istringstream istrs(forward_block);
    boost::archive::text_iarchive ar(istrs);
    ParameterStack deserialized;

    ar >> deserialized;

    return deserialized;
  } catch (const std::exception &) {
    return ParameterStack();
  }
}

LayerParameters make_next_forward_node_ciruit_layer_parameters(
    const ParameterStack &next_node_full_stack) {
  LayerParameters circuit_link_parameters;
  circuit_link_parameters["forward"] = "1";
  circuit_link_parameters["circuit_id"] = "";
  circuit_link_parameters["circuit_nodes"] =
      make_forward_block(next_node_full_stack);
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

  return CircuitEndpointContext({forward, id, forward_blocks, details});
}

}  // detail

class NodeParameterList {
 private:
  typedef std::list<ParameterStack> NodeList;

 public:
  void PushFrontNode(ParameterStack new_node_stack = ParameterStack()) {
    nodes_.push_back(std::move(new_node_stack));
  }

  ParameterStack &FrontNode() { return nodes_.back(); }
  const ParameterStack &FrontNode() const { return nodes_.back(); }
  void PopFrontNode() { nodes_.pop_back(); }

  void AddTopLayerToFrontNode(LayerParameters top_layer) {
    nodes_.back().push_front(std::move(top_layer));
  }

  void PushBackNode(ParameterStack new_node_stack = ParameterStack()) {
    nodes_.push_front(std::move(new_node_stack));
  }

  ParameterStack &BackNode() { return nodes_.front(); }
  const ParameterStack &BackNode() const { return nodes_.front(); }
  void PopBackNode() { nodes_.pop_front(); }

  void AddTopLayerToBackNode(LayerParameters top_layer) {
    nodes_.front().push_front(std::move(top_layer));
  }

  NodeList::iterator begin() { return nodes_.begin(); }
  NodeList::iterator end() { return nodes_.end(); }

  NodeList::const_iterator begin() const { return nodes_.begin(); }
  NodeList::const_iterator end() const { return nodes_.end(); }

 private:
  NodeList nodes_;
};

ParameterStack make_acceptor_parameter_stack(
    std::string local_id, ParameterStack next_layer_parameters) {
  LayerParameters circuit_link_parameters;
  circuit_link_parameters["forward"] = "0";
  circuit_link_parameters["circuit_id"] = std::move(local_id);
  circuit_link_parameters["circuit_nodes"] = "";
  circuit_link_parameters["details"] = "";

  ParameterStack acceptor_parameters(std::move(next_layer_parameters));
  acceptor_parameters.push_front(std::move(circuit_link_parameters));

  return acceptor_parameters;
}

ParameterStack make_forwarding_acceptor_parameter_stack(
    std::string local_id, ParameterStack next_layer_parameters) {
  LayerParameters circuit_link_parameters;
  circuit_link_parameters["forward"] = "1";
  circuit_link_parameters["circuit_id"] = std::move(local_id);
  circuit_link_parameters["circuit_nodes"] = "";
  circuit_link_parameters["details"] = "";

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

}  // data_link_layer
}  // virtual_network

#endif  // SSF_CORE_VIRTUAL_NETWORK_DATA_LINK_CIRCUIT_HELPERS_H_
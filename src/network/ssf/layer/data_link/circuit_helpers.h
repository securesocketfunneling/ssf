#ifndef SSF_LAYER_DATA_LINK_CIRCUIT_HELPERS_H_
#define SSF_LAYER_DATA_LINK_CIRCUIT_HELPERS_H_

#include <list>
#include <sstream>
#include <string>

#include <boost/asio/io_service.hpp>

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

}  // data_link
}  // layer
}  // ssf

#endif  // SSF_LAYER_DATA_LINK_CIRCUIT_HELPERS_H_
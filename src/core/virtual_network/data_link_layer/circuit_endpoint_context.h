#ifndef SSF_CORE_VIRTUAL_NETWORK_DATA_LINK_LAYER_CIRCUIT_ENDPOINT_CONTEXT_H_
#define SSF_CORE_VIRTUAL_NETWORK_DATA_LINK_LAYER_CIRCUIT_ENDPOINT_CONTEXT_H_

#include <cstdint>

#include <string>
#include <list>

namespace virtual_network {
namespace data_link_layer {

struct CircuitEndpointContext {
  typedef std::string ID;
  typedef std::string ForwardBlock;
  typedef std::string Details;

  bool operator==(const CircuitEndpointContext& rhs) const {
    return id == rhs.id;
  }

  bool operator!=(const CircuitEndpointContext& rhs) const {
    return !(*this == rhs);
  }

  bool operator<(const CircuitEndpointContext& rhs) const {
    return id < rhs.id;
  }

  bool forward;
  // TODO change std::string to uint32_t for id (?)
  ID id;
  ForwardBlock forward_blocks;
  Details details;
};

}  // data_link_layer
}  // virtual_network

#endif  // SSF_CORE_VIRTUAL_NETWORK_DATA_LINK_LAYER_CIRCUIT_ENDPOINT_CONTEXT_H_
#ifndef SSF_LAYER_DATA_LINK_CIRCUIT_ENDPOINT_CONTEXT_H_
#define SSF_LAYER_DATA_LINK_CIRCUIT_ENDPOINT_CONTEXT_H_

#include <cstdint>

#include <string>
#include <list>

namespace ssf {
namespace layer {
namespace data_link {

struct CircuitEndpointContext {
  using ID = std::string;
  using SerializedForwardBlocks = std::string;
  using Details = std::string;
  using SerializedDefaultParameters = std::string;

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
  SerializedForwardBlocks forward_blocks;
  SerializedDefaultParameters default_parameters;
  Details details;
};

}  // data_link
}  // layer
}  // ssf

#endif  // SSF_LAYER_DATA_LINK_CIRCUIT_ENDPOINT_CONTEXT_H_
#ifndef SSF_CORE_VIRTUAL_NETWORK_DATA_LINK_HELPERS_H_
#define SSF_CORE_VIRTUAL_NETWORK_DATA_LINK_HELPERS_H_

#include <cstdint>

#include <string>
#include <map>

#include "core/virtual_network/basic_endpoint.h"

namespace virtual_network {
namespace data_link_layer {
namespace detail {

template <class Protocol>
bool is_endpoint_forwarding(
    const basic_VirtualLink_endpoint<Protocol>& endpoint) {
  return endpoint.is_set() && endpoint.endpoint_context().forward;
}

}  // detail
}  // data_link_layer
}  // virtual_network

#endif  // SSF_CORE_VIRTUAL_NETWORK_DATA_LINK_HELPERS_H_
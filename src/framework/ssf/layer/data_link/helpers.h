#ifndef SSF_LAYER_DATA_LINK_HELPERS_H_
#define SSF_LAYER_DATA_LINK_HELPERS_H_

#include <cstdint>

#include <string>
#include <map>

#include "ssf/layer/basic_endpoint.h"

namespace ssf {
namespace layer {
namespace data_link {
namespace detail {

template <class Protocol>
bool is_endpoint_forwarding(
    const basic_VirtualLink_endpoint<Protocol>& endpoint) {
  return endpoint.is_set() && endpoint.endpoint_context().forward;
}

}  // detail
}  // data_link
}  // layer
}  // ssf

#endif  // SSF_LAYER_DATA_LINK_HELPERS_H_
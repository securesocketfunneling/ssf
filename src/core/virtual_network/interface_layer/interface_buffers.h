#ifndef SSF_CORE_VIRTUAL_NETWORK_INTERFACE_LAYER_INTERFACE_BUFFERS_H_
#define SSF_CORE_VIRTUAL_NETWORK_INTERFACE_LAYER_INTERFACE_BUFFERS_H_

#include <common/io/buffers.h>

namespace virtual_network {
namespace interface_layer {

typedef io::fixed_mutable_buffer_sequence interface_mutable_buffers;
typedef io::fixed_const_buffer_sequence interface_const_buffers;

}  // interface_layer
}  // virtual_network

#endif  // SSF_CORE_VIRTUAL_NETWORK_INTERFACE_LAYER_INTERFACE_BUFFERS_H_

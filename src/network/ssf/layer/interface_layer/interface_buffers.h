#ifndef SSF_LAYER_INTERFACE_LAYER_INTERFACE_BUFFERS_H_
#define SSF_LAYER_INTERFACE_LAYER_INTERFACE_BUFFERS_H_

#include "ssf/io/buffers.h"

namespace ssf {
namespace layer {
namespace interface_layer {

typedef io::fixed_mutable_buffer_sequence interface_mutable_buffers;
typedef io::fixed_const_buffer_sequence interface_const_buffers;

}  // interface_layer
}  // layer
}  // ssf

#endif  // SSF_LAYER_INTERFACE_LAYER_INTERFACE_BUFFERS_H_

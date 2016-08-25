#ifndef SSF_LAYER_DATAGRAM_EMPTY_COMPONENT_H_
#define SSF_LAYER_DATAGRAM_EMPTY_COMPONENT_H_

#include <cstdint>

#include <vector>

#include <boost/asio/buffer.hpp>

namespace ssf {
namespace layer {

  class EmptyComponent {
  public:
   typedef io::fixed_const_buffer_sequence ConstBuffers;
   typedef io::fixed_mutable_buffer_sequence MutableBuffers;
    enum { size = 0 };

  public:
    EmptyComponent() {}
    ~EmptyComponent() {}

    ConstBuffers GetConstBuffers() const { return ConstBuffers(); }
    void GetConstBuffers(ConstBuffers* p_buffers) const {}

    MutableBuffers GetMutableBuffers() { return MutableBuffers(); }
    void GetMutableBuffers(MutableBuffers* p_buffers) {}
  };

}  // layer
}  // ssf

#endif  // SSF_LAYER_DATAGRAM_EMPTY_COMPONENT_H_

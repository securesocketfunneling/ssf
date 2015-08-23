#ifndef SSF_CORE_VIRTUAL_NETWORK_DATAGRAM_EMPTY_COMPONENT_H_
#define SSF_CORE_VIRTUAL_NETWORK_DATAGRAM_EMPTY_COMPONENT_H_

#include <cstdint>

#include <vector>

#include <boost/asio/buffer.hpp>

namespace virtual_network {

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

}  // virtual_network

#endif  // SSF_CORE_VIRTUAL_NETWORK_DATAGRAM_EMPTY_COMPONENT_H_

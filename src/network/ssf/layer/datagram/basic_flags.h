#ifndef SSF_LAYER_DATAGRAM_BASIC_FLAGS_H_
#define SSF_LAYER_DATAGRAM_BASIC_FLAGS_H_

#include <cstdint>

#include <boost/asio/buffer.hpp>

namespace ssf {
namespace layer {

template <typename BasicType>
class basic_Flags {
 public:
  typedef io::fixed_const_buffer_sequence ConstBuffers;
  typedef io::fixed_mutable_buffer_sequence MutableBuffers;
  enum { size = sizeof(BasicType) };

 public:
  basic_Flags() {}
  basic_Flags(BasicType raw_flags) : raw_flags_(raw_flags) {}

  ~basic_Flags() {}

  ConstBuffers GetConstBuffers() const {
    ConstBuffers buffers;
    buffers.push_back(boost::asio::buffer(&raw_flags_, sizeof(raw_flags_)));
    return buffers;
  }

  void GetConstBuffers(ConstBuffers* p_buffers) const {
    p_buffers->push_back(boost::asio::buffer(&raw_flags_, sizeof(raw_flags_)));
  }

  MutableBuffers GetMutableBuffers() {
    MutableBuffers buffers;
    buffers.push_back(boost::asio::buffer(&raw_flags_, sizeof(raw_flags_)));
    return buffers;
  }

  void GetMutableBuffers(MutableBuffers* p_buffers) {
    p_buffers->push_back(boost::asio::buffer(&raw_flags_, sizeof(raw_flags_)));
  }

  BasicType raw_flags() const { return raw_flags_ }
  void set_raw_flags(BasicType raw_flags) { raw_flags_ = raw_flags; }

 private:
  BasicType raw_flags_;
};

}  // layer
}  // ssf

#endif  // SSF_LAYER_DATAGRAM_BASIC_FLAGS_H_

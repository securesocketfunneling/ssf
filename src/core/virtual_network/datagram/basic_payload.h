#ifndef SSF_CORE_VIRTUAL_NETWORK_DATAGRAM_BASIC_PAYLOAD_H_
#define SSF_CORE_VIRTUAL_NETWORK_DATAGRAM_BASIC_PAYLOAD_H_

#include <cstdint>

#include <vector>

#include <boost/asio/buffer.hpp>

#include "common/io/buffers.h"

namespace virtual_network {

class MutablePayload {
 public:
  typedef io::fixed_mutable_buffer_sequence MutableBuffers;
  typedef io::fixed_const_buffer_sequence ConstBuffers;
  enum { size = 0 };

 public:
  template <class MutableBufferSequence>
  MutablePayload(MutableBufferSequence&& buffers)
      : data_(std::forward<MutableBufferSequence>(buffers)) {}

  ~MutablePayload() {}

  MutableBuffers GetMutableBuffers() const { return data_; }

  void GetMutableBuffers(MutableBuffers* p_buffers) const {
    for (const auto& buffer : data_) {
      p_buffers->push_back(buffer);
    }
  }

  ConstBuffers GetConstBuffers() const { return data_; }

  void GetConstBuffers(ConstBuffers* p_buffers) const {
    for (const auto& buffer : data_) {
      p_buffers->push_back(buffer);
    }
  }

  uint32_t GetSize() const { return boost::asio::buffer_size(data_); }

  MutableBuffers::iterator begin() { return data_.begin(); }
  MutableBuffers::iterator end() { return data_.end(); }

  MutableBuffers::const_iterator begin() const { return data_.begin(); }
  MutableBuffers::const_iterator end() const { return data_.end(); }

 private:
  MutableBuffers data_;
};

class ConstPayload {
 public:
  typedef io::fixed_const_buffer_sequence ConstBuffers;
  enum { size = 0 };

 public:
  template <class ConstBufferSequence>
  ConstPayload(ConstBufferSequence&& buffers)
      : data_(std::forward<ConstBufferSequence>(buffers)) {}

  ~ConstPayload() {}

  ConstBuffers GetConstBuffers() const { return data_; }

  void GetConstBuffers(ConstBuffers* p_buffers) const {
    for (const auto& buffer : data_) {
      p_buffers->push_back(buffer);
    }
  }

  uint32_t GetSize() const { return boost::asio::buffer_size(data_); }

  ConstBuffers::iterator begin() { return data_.begin(); }
  ConstBuffers::iterator end() { return data_.end(); }

  ConstBuffers::const_iterator begin() const { return data_.begin(); }
  ConstBuffers::const_iterator end() const { return data_.end(); }

 private:
  ConstBuffers data_;
};

template <uint32_t MaxSize>
class BufferPayload {
 public:
  typedef io::fixed_const_buffer_sequence ConstBuffers;
  typedef io::fixed_mutable_buffer_sequence MutableBuffers;
  enum { size = 0 };

 public:
  BufferPayload() : data_(MaxSize) {}
  ~BufferPayload() {}

  ConstBuffers GetConstBuffers() const {
    return ConstBuffers({boost::asio::buffer(data_)});
  }

  void GetConstBuffers(ConstBuffers* p_buffers) const {
    p_buffers->push_back(boost::asio::buffer(data_));
  }

  MutableBuffers GetMutableBuffers() {
    return MutableBuffers({boost::asio::buffer(data_)});
  }

  void GetMutableBuffers(MutableBuffers* p_buffers) {
    p_buffers->push_back(boost::asio::buffer(data_));
  }

  uint32_t GetSize() const { return data_.size(); }

  void SetSize(uint32_t new_size) {
    if (new_size <= MaxSize) {
      data_.resize(new_size);
    }
  }

  void ResetSize() { data_.resize(MaxSize); }

 private:
  std::vector<uint8_t> data_;
};

}  // virtual_network

#endif  // SSF_CORE_VIRTUAL_NETWORK_DATAGRAM_BASIC_PAYLOAD_H_

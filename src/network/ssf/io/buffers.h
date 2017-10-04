#ifndef SSF_IO_BUFFERS_H_
#define SSF_IO_BUFFERS_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif  // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/buffer.hpp>

namespace ssf {
namespace io {

template <class BufferType>
class fixed_buffer_sequence {
 public:
  typedef std::vector<BufferType> buffer_type;
  typedef typename buffer_type::value_type value_type;
  typedef typename buffer_type::iterator iterator;
  typedef typename buffer_type::const_iterator const_iterator;

  fixed_buffer_sequence() : buffers_() {}

  template <class BufferSequence>
  fixed_buffer_sequence(const BufferSequence& buffers)
      : buffers_() {
    for (const auto& buffer : buffers) {
      buffers_.push_back(buffer);
    }
  }

  iterator begin() { return buffers_.begin(); }
  iterator end() { return buffers_.end(); }

  const_iterator begin() const { return buffers_.begin(); }
  const_iterator end() const { return buffers_.end(); }

  void push_back(const value_type& val) { buffers_.push_back(val); }
  void push_back(value_type&& val) { buffers_.push_back(std::move(val)); }

 private:
  buffer_type buffers_;
};

typedef fixed_buffer_sequence<boost::asio::mutable_buffer>
    fixed_mutable_buffer_sequence;

typedef fixed_buffer_sequence<boost::asio::const_buffer>
    fixed_const_buffer_sequence;

}  // io
}  // ssf

#endif  // SSF_IO_BUFFERS_H_

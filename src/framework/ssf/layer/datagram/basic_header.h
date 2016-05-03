#ifndef SSF_LAYER_DATAGRAM_BASIC_HEADER_H_
#define SSF_LAYER_DATAGRAM_BASIC_HEADER_H_

#include <cstdint>

#include <type_traits>

#include <boost/asio/buffer.hpp>

namespace ssf {
namespace layer {

template <class T>
struct ResolvePayloadLengthBaseSize {
  enum { value = sizeof(T) };
};

struct uint0_t {
  template <class T>
  uint0_t(const T&) {}
};

template <>
struct ResolvePayloadLengthBaseSize<uint0_t> {
  enum { value = 0 };
};

template <typename TVersion, class TID, class TFlags, class TPayloadLength>
class basic_Header {
 public:
  typedef TVersion Version;
  typedef TID ID;
  typedef TFlags Flags;
  typedef TPayloadLength PayloadLength;

 private:
  enum {
    payload_length_base_size =
        ResolvePayloadLengthBaseSize<PayloadLength>::value
  };

 public:
  typedef io::fixed_const_buffer_sequence ConstBuffers;
  typedef io::fixed_mutable_buffer_sequence MutableBuffers;
  enum {
    size = Version::size + ID::size + Flags::size + payload_length_base_size
  };

 public:
  basic_Header() : version_(), id_(), flags_(), payload_length_(0) {}

  basic_Header(Version version, ID id, Flags flags,
               PayloadLength payload_length)
      : version_(std::move(version)),
        id_(std::move(id)),
        flags_(std::move(flags)),
        payload_length_(std::move(payload_length)) {}

  ~basic_Header() {}

  ConstBuffers GetConstBuffers() const {
    ConstBuffers buffers;
    version_.GetConstBuffers(&buffers);
    id_.GetConstBuffers(&buffers);
    flags_.GetConstBuffers(&buffers);
    buffers.push_back(
        boost::asio::buffer(&payload_length_, payload_length_base_size));
    return buffers;
  }

  void GetConstBuffers(ConstBuffers* p_buffers) const {
    version_.GetConstBuffers(p_buffers);
    id_.GetConstBuffers(p_buffers);
    flags_.GetConstBuffers(p_buffers);
    p_buffers->push_back(
        boost::asio::buffer(&payload_length_, payload_length_base_size));
  }

  MutableBuffers GetMutableBuffers() {
    MutableBuffers buffers;
    version_.GetMutableBuffers(&buffers);
    id_.GetMutableBuffers(&buffers);
    flags_.GetMutableBuffers(&buffers);
    buffers.push_back(
        boost::asio::buffer(&payload_length_, payload_length_base_size));
    return buffers;
  }

  void GetMutableBuffers(MutableBuffers* p_buffers) {
    version_.GetMutableBuffers(p_buffers);
    id_.GetMutableBuffers(p_buffers);
    flags_.GetMutableBuffers(p_buffers);
    p_buffers->push_back(
        boost::asio::buffer(&payload_length_, payload_length_base_size));
  }

  Version& version() { return version_; }
  const Version& version() const { return version_; }

  ID& id() { return id_; }
  const ID& id() const { return id_; }

  Flags& flags() { return flags_; }
  const Flags& flags() const { return flags_; }

  PayloadLength& payload_length() { return payload_length_; }
  const PayloadLength& payload_length() const { return payload_length_; }

 private:
  Version version_;
  ID id_;
  Flags flags_;
  PayloadLength payload_length_;
};

}  // layer
}  // ssf

#endif  // SSF_LAYER_DATAGRAM_BASIC_HEADER_H_

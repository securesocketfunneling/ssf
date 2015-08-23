#ifndef SSF_CORE_VIRTUAL_NETWORK_DATAGRAM_BASIC_DATAGRAM_H_
#define SSF_CORE_VIRTUAL_NETWORK_DATAGRAM_BASIC_DATAGRAM_H_

#include <cstdint>

#include <boost/asio/buffer.hpp>

#include "common/io/buffers.h"

namespace virtual_network {

template <class THeader, class TPayload, class TFooter>
class basic_Datagram {
 public:
  typedef THeader Header;
  typedef TPayload Payload;
  typedef TFooter Footer;

 public:
  typedef io::fixed_const_buffer_sequence ConstBuffers;
  typedef io::fixed_mutable_buffer_sequence MutableBuffers;
  enum { size = Header::size + Payload::size + Footer::size };

 public:
  basic_Datagram() : header_(), payload_(), footer_() {}

  basic_Datagram(Header header, Payload payload, Footer footer)
      : header_(std::move(header)),
        payload_(std::move(payload)),
        footer_(std::move(footer)) {}

  template <class OtherPayload>
  basic_Datagram(basic_Datagram<Header, OtherPayload, Footer> datagram)
      : header_(std::move(datagram.header)),
        payload_(std::move(datagram.payload)),
        footer_(std::move(datagram.footer)) {}

  ~basic_Datagram() {}

  ConstBuffers GetConstBuffers() const {
    ConstBuffers buffers;
    header_.GetConstBuffers(&buffers);
    payload_.GetConstBuffers(&buffers);
    footer_.GetConstBuffers(&buffers);
    return buffers;
  }

  void GetConstBuffers(ConstBuffers* p_buffers) const {
    header_.GetConstBuffers(p_buffers);
    payload_.GetConstBuffers(p_buffers);
    footer_.GetConstBuffers(p_buffers);
  }

  MutableBuffers GetMutableBuffers() {
    MutableBuffers buffers;
    header_.GetMutableBuffers(&buffers);
    payload_.GetMutableBuffers(&buffers);
    footer_.GetMutableBuffers(&buffers);
    return buffers;
  }

  void GetMutableBuffers(MutableBuffers* p_buffers) {
    header_.GetMutableBuffers(p_buffers);
    payload_.GetMutableBuffers(p_buffers);
    footer_.GetMutableBuffers(p_buffers);
  }

  Header& header() { return header_; }
  const Header& header() const { return header_; }

  Payload& payload() { return payload_; }
  const Payload& payload() const { return payload_; }

  Footer& footer() { return footer_; }
  const Footer& footer() const { return footer_; }

 private:
  Header header_;
  Payload payload_;
  Footer footer_;
};

}  // virtual_network

#endif  // SSF_CORE_VIRTUAL_NETWORK_DATAGRAM_BASIC_DATAGRAM_H_

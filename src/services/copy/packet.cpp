#include "services/copy/packet.h"

namespace ssf {
namespace services {
namespace copy {

Packet::Packet() : type_(PacketType::kUnknown), payload_size_(0), buffer_() {}

PacketType Packet::type() const { return type_; }

void Packet::set_type(PacketType type) { type_ = type; }

uint32_t Packet::payload_size() const { return payload_size_; }

void Packet::set_payload_size(uint32_t size) { payload_size_ = size; }

const Packet::Buffer& Packet::buffer() const { return buffer_; }

Packet::Buffer& Packet::buffer() { return buffer_; }

Packet::ConstBufSeq Packet::GetConstBuf() const {
  return {boost::asio::buffer(&type_, sizeof(type_)),
          boost::asio::buffer(&payload_size_, sizeof(payload_size_)),
          boost::asio::buffer(buffer_, payload_size_)};
}

Packet::HeaderConstBufSeq Packet::GetHeaderConstBuf() const {
  return {boost::asio::buffer(&type_, sizeof(type_)),
          boost::asio::buffer(&payload_size_, sizeof(payload_size_))};
}

Packet::HeaderMutBufSeq Packet::GetHeaderMutBuf() {
  return {boost::asio::buffer(&type_, sizeof(type_)),
          boost::asio::buffer(&payload_size_, sizeof(payload_size_))};
}

boost::asio::mutable_buffers_1 Packet::GetPayloadMutBuf() {
  return {boost::asio::buffer(buffer_, payload_size_)};
}

}  // copy
}  // services
}  // ssf

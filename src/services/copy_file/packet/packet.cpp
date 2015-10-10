#include "services/copy_file/packet/packet.h"

namespace ssf {
namespace services {
namespace copy_file {
namespace packet {

Packet::Packet() : type_(kCtrl), signal_(kInterrupted), size_(0), buffer_() {}

Packet::Type Packet::type() const { return type_; }

void Packet::set_type(Type type) { type_ = type; }

boost::asio::const_buffers_1 Packet::GetTypeConstBuf() const {
  return boost::asio::const_buffers_1(
      boost::asio::buffer(&type_, sizeof(type_)));
}

boost::asio::mutable_buffers_1 Packet::GetTypeMutBuf() {
  return boost::asio::buffer(&type_, sizeof(type_));
}

bool Packet::IsDataPacket() { return type_ == kData; }

Packet::Size Packet::size() const { return size_; }

void Packet::set_size(Size size) { size_ = size; }

boost::asio::const_buffers_1 Packet::GetSizeConstBuf() const {
  return boost::asio::const_buffers_1(
      boost::asio::buffer(&size_, sizeof(size_)));
}

boost::asio::mutable_buffers_1 Packet::GetSizeMutBuf() {
  return boost::asio::buffer(&size_, sizeof(size_));
}

Packet::Signal Packet::signal() const { return signal_; }

void Packet::set_signal(Signal signal) { signal_ = signal; }

boost::asio::const_buffers_1 Packet::GetSignalConstBuf() const {
  return boost::asio::const_buffers_1(
      boost::asio::buffer(&signal_, sizeof(signal_)));
}

boost::asio::mutable_buffers_1 Packet::GetSignalMutBuf() {
  return boost::asio::buffer(&signal_, sizeof(signal_));
}

Packet::Buffer& Packet::buffer() { return buffer_; }

Packet::ConstBufSeq Packet::GetConstBuf() const {
  ConstBufSeq buf_seq;
  auto header_buf = GetHeaderConstBuf();
  auto payload_buf = GetPayloadConstBuf();

  buf_seq.insert(buf_seq.end(), header_buf.begin(), header_buf.end());
  buf_seq.insert(buf_seq.end(), payload_buf.begin(), payload_buf.end());

  return buf_seq;
}

Packet::ConstBufSeq Packet::GetHeaderConstBuf() const {
  ConstBufSeq buf_seq;
  buf_seq.push_back(boost::asio::buffer(&type_, sizeof(type_)));
  if (type_ == kData) {
    buf_seq.push_back(boost::asio::buffer(&size_, sizeof(size_)));
  }

  return buf_seq;
}

Packet::ConstBufSeq Packet::GetPayloadConstBuf() const {
  ConstBufSeq buf_seq;
  if (type_ == kCtrl) {
    buf_seq.push_back(boost::asio::buffer(&signal_, sizeof(signal_)));
  } else {
    buf_seq.push_back(boost::asio::buffer(buffer_, size_));
  }

  return buf_seq;
}

Packet::MutBufSeq Packet::GetPayloadMutBuf() {
  MutBufSeq buf_seq;
  if (type_ == kCtrl) {
    buf_seq.push_back(boost::asio::buffer(&signal_, sizeof(signal_)));
  } else {
    buf_seq.push_back(boost::asio::buffer(buffer_, size_));
  }

  return buf_seq;
}

}  // packet
}  // copy_file
}  // services
}  // ssf

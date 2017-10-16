#ifndef SSF_SERVICES_COPY_PACKET_H_
#define SSF_SERVICES_COPY_PACKET_H_

#include <cstdint>

#include <array>
#include <memory>

#include <boost/asio/buffer.hpp>

namespace ssf {
namespace services {
namespace copy {

enum class PacketType : uint32_t {
  // copy session
  kUnknown = 0,
  kInitRequest,
  kInitReply,
  kCheckIntegrityRequest,
  kCheckIntegrityReply,
  kData,
  kEof,
  kAbort,
  kAbortAck,
  // control channel
  kCopyRequest,
  CopyRequestAck,
  kCopyFinished
};

class Packet {
 public:
  static const uint64_t kMaxPayloadSize = 50 * 1024;
  using Buffer = std::array<char, kMaxPayloadSize>;

  using ConstBufSeq = std::array<boost::asio::const_buffer, 3>;
  using HeaderConstBufSeq = std::array<boost::asio::const_buffer, 2>;
  using HeaderMutBufSeq = std::array<boost::asio::mutable_buffer, 2>;

 public:
  Packet();
  PacketType type() const;
  void set_type(PacketType type);
  boost::asio::const_buffers_1 GetTypeConstBuf() const;
  boost::asio::mutable_buffers_1 GetTypeMutBuf();

  uint32_t payload_size() const;
  void set_payload_size(uint32_t size);
  const Buffer& buffer() const;
  Buffer& buffer();

  ConstBufSeq GetConstBuf() const;
  HeaderConstBufSeq GetHeaderConstBuf() const;
  HeaderMutBufSeq GetHeaderMutBuf();
  boost::asio::mutable_buffers_1 GetPayloadMutBuf();

 private:
  PacketType type_;
  uint32_t payload_size_;
  Buffer buffer_;
};

using PacketPtr = std::shared_ptr<Packet>;

}  // copy
}  // services
}  // ssf

#endif  // SSF_SERVICES_COPY_PACKET_H_

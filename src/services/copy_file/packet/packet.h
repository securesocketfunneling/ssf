#ifndef SRC_SERVICES_COPY_FILE_PACKET_PACKET_H_
#define SRC_SERVICES_COPY_FILE_PACKET_PACKET_H_

#include <cstdint>

#include <array>

#include <boost/asio/buffer.hpp>

namespace ssf {
namespace services {
namespace copy_file {
namespace packet {

class Packet {
 public:
  using Size = uint32_t;
  using Buffer = std::array<char, 50 * 1024>;

  using ConstBufSeq = std::vector<boost::asio::const_buffer>;
  using MutBufSeq = std::vector<boost::asio::mutable_buffer>;

  enum Type : uint8_t { kCtrl = 0, kData = 1 };

  enum Signal : uint8_t { kEof = 0, kInterrupted = 1 };

 public:
  Packet();
  Type type() const;
  void set_type(Type type);
  boost::asio::const_buffers_1 GetTypeConstBuf() const;
  boost::asio::mutable_buffers_1 GetTypeMutBuf();

  bool IsDataPacket();

  Size size() const;
  void set_size(Size size);
  boost::asio::const_buffers_1 GetSizeConstBuf() const;
  boost::asio::mutable_buffers_1 GetSizeMutBuf();

  Signal signal() const;
  void set_signal(Signal signal);
  boost::asio::const_buffers_1 GetSignalConstBuf() const;
  boost::asio::mutable_buffers_1 GetSignalMutBuf();

  Buffer& buffer();

  ConstBufSeq GetConstBuf() const;

  ConstBufSeq GetHeaderConstBuf() const;

  ConstBufSeq GetPayloadConstBuf() const;
  MutBufSeq GetPayloadMutBuf();

 private:
  Type type_;
  Signal signal_;
  Size size_;
  Buffer buffer_;
};

}  // packet
}  // copy_file
}  // services
}  // ssf

#endif  // SRC_SERVICES_COPY_FILE_PACKET_PACKET_H_

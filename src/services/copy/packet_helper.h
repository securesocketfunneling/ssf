#ifndef SSF_SERVICES_COPY_PACKET_READER_H_
#define SSF_SERVICES_COPY_PACKET_READER_H_

#include <cstdint>

#include <array>

#include <boost/asio/buffer.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/system/error_code.hpp>

#include <msgpack.hpp>

#include <ssf/log/log.h>

#include "common/error/error.h"

#include "services/copy/packet.h"

namespace ssf {
namespace services {
namespace copy {

using OnPacketRead = std::function<void(const boost::system::error_code& ec)>;
using OnPayloadSent = std::function<void(const boost::system::error_code& ec)>;

template <class Socket>
void AsyncReadPayload(Socket& socket, Packet& packet,
                      OnPacketRead on_packet_read) {
  auto on_payload_received = [&socket, on_packet_read](
      const boost::system::error_code& ec, std::size_t length) {
    on_packet_read(ec);
  };
  boost::asio::async_read(socket, packet.GetPayloadMutBuf(),
                          on_payload_received);
}

template <class Socket>
void AsyncReadHeader(Socket& socket, Packet& packet,
                     OnPacketRead on_packet_read) {
  auto on_header_received = [&socket, &packet, on_packet_read](
      const boost::system::error_code& ec, std::size_t length) {
    if (ec) {
      SSF_LOG("microservice", debug,
              "[copy][packet_helper] cannot read packet header");
      on_packet_read(ec);
      return;
    }
    AsyncReadPayload(socket, packet, on_packet_read);
  };
  boost::asio::async_read(socket, packet.GetHeaderMutBuf(), on_header_received);
}

template <class Socket>
void AsyncReadPacket(Socket& socket, Packet& packet,
                     OnPacketRead on_packet_read) {
  AsyncReadHeader(socket, packet, on_packet_read);
}

template <class Socket, class Payload>
void AsyncWritePayload(Socket& socket, const Payload& payload, PacketPtr packet,
                       OnPayloadSent on_payload_sent) {
  boost::system::error_code convert_ec;
  PayloadToPacket(payload, packet.get(), convert_ec);
  if (convert_ec) {
    SSF_LOG("microservice", debug,
            "[copy][packet_helper] cannot convert payload into packet");
    socket.get_io_service().post(
        [convert_ec, on_payload_sent]() { on_payload_sent(convert_ec); });
    return;
  }

  auto on_packet_sent = [packet, on_payload_sent](
      const boost::system::error_code& ec, std::size_t len) {
    on_payload_sent(ec);
  };
  boost::asio::async_write(socket, packet->GetConstBuf(), on_packet_sent);
}

template <class Payload>
void PayloadToPacket(const Payload& payload, Packet* packet,
                     boost::system::error_code& ec) {
  msgpack::sbuffer req_buf;
  msgpack::pack(req_buf, payload);

  if (req_buf.size() > Packet::kMaxPayloadSize) {
    SSF_LOG("microservice", debug,
            "[copy][packet_helper] could not convert payload to packet (size "
            "error)");
    ec.assign(::error::protocol_error, ::error::get_ssf_category());
    return;
  }

  packet->set_payload_size(req_buf.size());
  std::copy(req_buf.data(), req_buf.data() + req_buf.size(),
            packet->buffer().begin());

  packet->set_type(Payload::kType);
}

template <class Payload>
void PacketToPayload(const Packet& packet, Payload& payload,
                     boost::system::error_code& ec) {
  try {
    auto obj_handle =
        msgpack::unpack(packet.buffer().data(), packet.payload_size());
    auto obj = obj_handle.get();
    obj.convert(payload);
  } catch (const std::exception& e) {
    (void)(e);
    SSF_LOG("microservice", debug,
            "[copy][packet_helper] could not convert "
            "packet to payload {}",
            e.what());
    ec.assign(::error::invalid_argument, ::error::get_ssf_category());
    return;
  }
}

}  // copy
}  // services
}  // ssf

#endif  // SSF_SERVICES_COPY_PACKET_READER_H_

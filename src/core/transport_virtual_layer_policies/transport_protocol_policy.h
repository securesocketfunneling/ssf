#ifndef SSF_CORE_TRANSPORT_VIRTUAL_LAYER_POLICIES_TRANSPORT_PROTOCOL_POLICY_H
#define SSF_CORE_TRANSPORT_VIRTUAL_LAYER_POLICIES_TRANSPORT_PROTOCOL_POLICY_H

#include <cstdint>

#include <functional>
#include <memory>

#include <boost/archive/basic_archive.hpp>
#include <boost/system/error_code.hpp>
#include <boost/log/trivial.hpp>

#include "common/error/error.h"

#include "core/transport_virtual_layer_policies/init_packets/ssf_reply.h"
#include "core/transport_virtual_layer_policies/init_packets/ssf_request.h"

#include "versions.h"

namespace ssf {

template <typename Socket>
class TransportProtocolPolicy {
 private:
  typedef std::shared_ptr<Socket> p_socket_type;
  typedef std::function<void(p_socket_type, const boost::system::error_code&)>
      callback_type;

 public:
   TransportProtocolPolicy(callback_type callback) : callback_(callback) {}

  void DoSSFInitiate(p_socket_type p_socket) {
    BOOST_LOG_TRIVIAL(info) << "transport: starting SSF protocol";

    uint32_t version = GetVersion();
    auto p_ssf_request = std::make_shared<SSFRequest>(version);

    boost::asio::async_write(
        *p_socket, p_ssf_request->const_buffer(),
        boost::bind(&TransportProtocolPolicy<Socket>::DoSSFValidReceive, this,
                    p_ssf_request, p_socket, _1, _2));
  }

  void DoSSFInitiateReceive(p_socket_type p_socket) {
    auto p_ssf_request = std::make_shared<SSFRequest>();
    boost::asio::async_read(
        *p_socket, p_ssf_request->buffer(),
        boost::bind(&TransportProtocolPolicy<Socket>::DoSSFValid, this,
                    p_ssf_request, p_socket, _1, _2));
  }

  void DoSSFValid(SSFRequestPtr p_ssf_request, p_socket_type p_socket,
                   const boost::system::error_code& ec, size_t length) {
    if (!ec) {
      uint32_t version = p_ssf_request->version();

      BOOST_LOG_TRIVIAL(trace) << "transport: SSF version read: " << version;

      if (IsSupportedVersion(version)) {
        auto p_ssf_reply = std::make_shared<SSFReply>(true);
        boost::asio::async_write(
            *p_socket, p_ssf_reply->const_buffer(),
            boost::bind(
            &TransportProtocolPolicy<Socket>::DoSSFProtocolFinished, this,
                p_ssf_reply, p_socket, _1, _2));
      } else {
        BOOST_LOG_TRIVIAL(error) << "transport: SSF version NOT supported " << version;
        boost::system::error_code result_ec(::error::wrong_protocol_type,
                                            ::error::get_ssf_category());
        callback_(p_socket, result_ec);
      }
    } else {
      BOOST_LOG_TRIVIAL(error) << "transport: SSF version NOT read " << ec.message();
      callback_(p_socket, ec);
    }
  }

  void DoSSFValidReceive(SSFRequestPtr p_ssf_request, p_socket_type p_socket,
                           const boost::system::error_code& ec, size_t length) {
    if (!ec) {
      BOOST_LOG_TRIVIAL(info) << "transport: SSF request sent";

      auto p_ssf_reply = std::make_shared<SSFReply>();

      boost::asio::async_read(
          *p_socket, p_ssf_reply->buffer(),
          boost::bind(&TransportProtocolPolicy<Socket>::DoSSFProtocolFinished,
                      this, p_ssf_reply, p_socket, _1, _2));
    } else {
      BOOST_LOG_TRIVIAL(error) << "transport: could NOT send the SSF request "
                               << ec.message();
      callback_(p_socket, ec);
    }
  }

  void DoSSFProtocolFinished(SSFReplyPtr p_ssf_reply, p_socket_type p_socket,
                               const boost::system::error_code& ec,
                               size_t length) {
    if (!ec) {
      if (p_ssf_reply->result()) {
        BOOST_LOG_TRIVIAL(info) << "transport: SSF reply OK";
        callback_(p_socket, ec);
      } else {
        boost::system::error_code result_ec(::error::wrong_protocol_type,
                                            ::error::get_ssf_category());
        BOOST_LOG_TRIVIAL(error) << "transport: SSF reply NOT ok " << ec.message();
        callback_(p_socket, result_ec);
      }
    } else {
      BOOST_LOG_TRIVIAL(error) << "transport: could NOT read SSF reply " << ec.message();
      callback_(p_socket, ec);
    }
  }

  uint32_t GetVersion() {
    uint32_t version = versions::major;
    version = version << 8;

    version |= versions::minor;
    version = version << 8;

    version |= versions::transport;
    version = version << 8;

    version |= uint8_t(boost::archive::BOOST_ARCHIVE_VERSION());

    return version;
  }

  bool IsSupportedVersion(uint32_t input_version) {
    boost::archive::library_version_type serialization(input_version &
      0x000000FF);
    input_version = input_version >> 8;

    uint8_t transport = (input_version & 0x000000FF);
    input_version = input_version >> 8;

    uint8_t minor = (input_version & 0x000000FF);
    input_version = input_version >> 8;

    uint8_t major = (input_version & 0x000000FF);

    return (major == versions::major) && (minor == versions::minor) &&
      (transport == versions::transport) &&
      (serialization == boost::archive::BOOST_ARCHIVE_VERSION());
  }

 private:
  callback_type callback_;
};
}  //ssf

#endif  // SSF_CORE_TRANSPORT_VIRTUAL_LAYER_POLICIES_TRANSPORT_PROTOCOL_POLICY_H

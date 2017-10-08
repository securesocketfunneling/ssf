#ifndef SSF_CORE_TRANSPORT_VIRTUAL_LAYER_POLICIES_TRANSPORT_PROTOCOL_POLICY_H
#define SSF_CORE_TRANSPORT_VIRTUAL_LAYER_POLICIES_TRANSPORT_PROTOCOL_POLICY_H

#include <cstdint>

#include <functional>
#include <memory>

#include <ssf/log/log.h>

#include <boost/system/error_code.hpp>

#include <ssf/log/log.h>

#include "common/error/error.h"

#include "core/transport_virtual_layer_policies/init_packets/ssf_reply.h"
#include "core/transport_virtual_layer_policies/init_packets/ssf_request.h"

#include "versions.h"

namespace ssf {

template <typename Socket>
class TransportProtocolPolicy {
 private:
  using SocketPtr = std::shared_ptr<Socket>;
  using TransportCb =
      std::function<void(SocketPtr, const boost::system::error_code&)>;

 public:
  TransportProtocolPolicy() {}

  virtual ~TransportProtocolPolicy() {}

  void DoSSFInitiate(SocketPtr p_socket, TransportCb callback) {
    SSF_LOG(kLogDebug) << "transport: starting SSF protocol";

    uint32_t version = GetVersion();
    auto p_ssf_request = std::make_shared<SSFRequest>(version);

    auto on_write = [this, p_ssf_request, p_socket, callback](
        const boost::system::error_code& ec, size_t length) {
      DoSSFValidReceive(p_ssf_request, p_socket, callback, ec, length);
    };
    boost::asio::async_write(*p_socket, p_ssf_request->const_buffer(),
                             on_write);
  }

  void DoSSFInitiateReceive(SocketPtr p_socket, TransportCb callback) {
    auto p_ssf_request = std::make_shared<SSFRequest>();

    auto on_read = [this, p_ssf_request, p_socket, callback](
        const boost::system::error_code& ec, size_t length) {
      DoSSFValid(p_ssf_request, p_socket, callback, ec, length);
    };
    boost::asio::async_read(*p_socket, p_ssf_request->buffer(), on_read);
  }

  void DoSSFValid(SSFRequestPtr p_ssf_request, SocketPtr p_socket,
                  TransportCb callback, const boost::system::error_code& ec,
                  size_t length) {
    if (ec) {
      SSF_LOG(kLogError) << "transport: SSF version NOT read " << ec.message();
      callback(p_socket, ec);
      return;
    }

    uint32_t version = p_ssf_request->version();

    SSF_LOG(kLogTrace) << "transport: SSF version read: " << version;

    if (!IsSupportedVersion(version)) {
      SSF_LOG(kLogError) << "transport: SSF version NOT supported " << version;
      boost::system::error_code result_ec(::error::wrong_protocol_type,
                                          ::error::get_ssf_category());
      callback(p_socket, result_ec);
      return;
    }

    auto p_ssf_reply = std::make_shared<SSFReply>(true);
    auto on_write = [this, p_ssf_reply, p_socket, callback](
        const boost::system::error_code& ec, size_t length) {
      DoSSFProtocolFinished(p_ssf_reply, p_socket, callback, ec, length);
    };
    boost::asio::async_write(*p_socket, p_ssf_reply->const_buffer(), on_write);
  }

  void DoSSFValidReceive(SSFRequestPtr p_ssf_request, SocketPtr p_socket,
                         TransportCb callback,
                         const boost::system::error_code& ec, size_t length) {
    if (ec) {
      SSF_LOG(kLogError) << "transport: could NOT send the SSF request "
                         << ec.message();
      callback(p_socket, ec);
      return;
    }

    SSF_LOG(kLogDebug) << "transport: SSF request sent";

    auto p_ssf_reply = std::make_shared<SSFReply>();

    auto on_read = [this, p_ssf_reply, p_socket, callback](
        const boost::system::error_code& ec, size_t length) {
      DoSSFProtocolFinished(p_ssf_reply, p_socket, callback, ec, length);
    };
    boost::asio::async_read(*p_socket, p_ssf_reply->buffer(), on_read);
  }

  void DoSSFProtocolFinished(SSFReplyPtr p_ssf_reply, SocketPtr p_socket,
                             TransportCb callback,
                             const boost::system::error_code& ec,
                             size_t length) {
    if (ec) {
      SSF_LOG(kLogError) << "transport: could NOT read SSF reply "
                         << ec.message();
      callback(p_socket, ec);
      return;
    }
    if (!p_ssf_reply->result()) {
      boost::system::error_code result_ec(::error::wrong_protocol_type,
                                          ::error::get_ssf_category());
      SSF_LOG(kLogError) << "transport: SSF reply NOT ok " << ec.message();
      callback(p_socket, result_ec);
      return;
    }
    SSF_LOG(kLogDebug) << "transport: SSF reply OK";
    callback(p_socket, ec);
  }

  uint32_t GetVersion() {
    uint32_t version = versions::major;
    version = version << 8;

    version |= versions::minor;
    version = version << 8;

    version |= versions::transport;
    version = version << 8;

    version |= versions::circuit;

    return version;
  }

  bool IsSupportedVersion(uint32_t input_version) {
    uint8_t circuit = (input_version & 0x000000FF);
    input_version = input_version >> 8;

    uint8_t transport = (input_version & 0x000000FF);
    input_version = input_version >> 8;

    uint8_t minor = (input_version & 0x000000FF);
    input_version = input_version >> 8;

    uint8_t major = (input_version & 0x000000FF);

    return (major == versions::major) && (transport == versions::transport);
  }
};

}  // ssf

#endif  // SSF_CORE_TRANSPORT_VIRTUAL_LAYER_POLICIES_TRANSPORT_PROTOCOL_POLICY_H

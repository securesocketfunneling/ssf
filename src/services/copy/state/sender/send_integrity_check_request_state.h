#ifndef SSF_SERVICES_COPY_STATE_SENDER_SEND_INTEGRITY_CHECK_REQUEST_STATE_H_
#define SSF_SERVICES_COPY_STATE_SENDER_SEND_INTEGRITY_CHECK_REQUEST_STATE_H_

#include <iostream>

#include <msgpack.hpp>

#include <ssf/log/log.h>

#include "common/crypto/hash.h"
#include "common/error/error.h"

#include "services/copy/i_copy_state.h"
#include "services/copy/packet/check.h"
#include "services/copy/state/on_abort.h"
#include "services/copy/state/sender/abort_sender_state.h"
#include "services/copy/state/sender/wait_integrity_check_reply_state.h"

namespace ssf {
namespace services {
namespace copy {

class SendIntegrityCheckRequestState : ICopyState {
 public:
  template <typename... Args>
  static ICopyStateUPtr Create(Args&&... args) {
    return ICopyStateUPtr(
        new SendIntegrityCheckRequestState(std::forward<Args>(args)...));
  }

 private:
  SendIntegrityCheckRequestState() : ICopyState() {}

 public:
  // ICopyState
  void Enter(CopyContext* context, boost::system::error_code& ec) {
    SSF_LOG(kLogDebug)
        << "microservice[copy][send_integrity_check_request] enter";
  }

  bool FillOutboundPacket(CopyContext* context, Packet* packet,
                          boost::system::error_code& ec) {
    boost::system::error_code hash_ec;

    auto digest = ssf::crypto::HashFile<CopyContext::Hash>(
        context->input_filepath, hash_ec);
    if (hash_ec) {
      SSF_LOG(kLogDebug) << "microservice[copy][send_integrity_check_request] "
                            "cannot generate input file digest";
      context->SetState(
          AbortSenderState::Create(ErrorCode::kInputFileDigestNotAvailable));
      return false;
    }

    CheckIntegrityRequest<CopyContext::Hash> req(digest);
    boost::system::error_code convert_ec;
    PayloadToPacket(req, packet, convert_ec);
    if (convert_ec) {
      SSF_LOG(kLogDebug) << "microservice[copy][send_integrity_check_request] "
                            "cannot convert integrity check request to packet";
      context->SetState(AbortSenderState::Create(
          ErrorCode::kIntegrityCheckRequestPacketNotGenerated));
      return false;
    }

    context->SetState(WaitIntegrityCheckReplyState::Create());
    return true;
  }

  void ProcessInboundPacket(CopyContext* context, const Packet& packet,
                            boost::system::error_code& ec) {
    if (packet.type() == PacketType::kAbort) {
      return OnSenderAbortPacket(context, packet, ec);
    }

    SSF_LOG(kLogDebug) << "microservice[copy][send_integrity_check_request] "
                          "cannot process inbound packet";
    context->SetState(
        AbortSenderState::Create(ErrorCode::kInboundPacketNotSupported));
  }

  bool IsTerminal(CopyContext* context) { return false; }
};

}  // copy
}  // services
}  // ssf

#endif  // SSF_SERVICES_COPY_STATE_SENDER_SEND_INTEGRITY_CHECK_REQUEST_STATE_H_

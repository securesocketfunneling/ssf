#ifndef SSF_SERVICES_COPY_STATE_SENDER_WAIT_INTEGRITY_CHECK_REPLY_STATE_H_
#define SSF_SERVICES_COPY_STATE_SENDER_WAIT_INTEGRITY_CHECK_REPLY_STATE_H_

#include <iostream>

#include <msgpack.hpp>

#include <ssf/log/log.h>

#include "common/error/error.h"

#include "services/copy/i_copy_state.h"
#include "services/copy/packet/init.h"
#include "services/copy/state/on_abort.h"
#include "services/copy/state/sender/abort_sender_state.h"
#include "services/copy/state/sender/close_state.h"

namespace ssf {
namespace services {
namespace copy {

class WaitIntegrityCheckReplyState : ICopyState {
 public:
  template <typename... Args>
  static ICopyStateUPtr Create(Args&&... args) {
    return ICopyStateUPtr(
        new WaitIntegrityCheckReplyState(std::forward<Args>(args)...));
  }

 private:
  WaitIntegrityCheckReplyState() : ICopyState() {}

 public:
  // ICopyState
  void Enter(CopyContext* context, boost::system::error_code& ec) {
    SSF_LOG(kLogTrace)
        << "microservice[copy][wait_integrity_check_reply] enter";
  }

  bool FillOutboundPacket(CopyContext* context, Packet* packet,
                          boost::system::error_code& ec) {
    return false;
  }

  void ProcessInboundPacket(CopyContext* context, const Packet& packet,
                            boost::system::error_code& ec) {
    if (packet.type() == PacketType::kAbort) {
      return OnSenderAbortPacket(context, packet, ec);
    }

    if (packet.type() != PacketType::kCheckIntegrityReply) {
      SSF_LOG(kLogDebug) << "microservice[copy][wait_integrity_check_reply] "
                            "cannot process inbound packet";
      context->SetState(
          AbortSenderState::Create(ErrorCode::kInboundPacketNotSupported));
      return;
    }

    CheckIntegrityReply<CopyContext::Hash> rep;

    boost::system::error_code convert_ec;
    PacketToPayload(packet, rep, convert_ec);
    if (convert_ec) {
      SSF_LOG(kLogDebug) << "microservice[copy][wait_integrity_check_reply] "
                            "cannot convert packet to integrity check reply";
      context->SetState(AbortSenderState::Create(
          ErrorCode::kIntegrityCheckReplyPacketCorrupted));
      return;
    }

    if (rep.status == CheckIntegrityStatus::kCheckIntegrityFailed) {
      SSF_LOG(kLogDebug) << "microservice[copy][wait_integrity_check_reply] "
                            "file integrity error";
      context->SetState(
          AbortSenderState::Create(ErrorCode::kOutputFileCorrupted));
      return;
    }

    // file transmitted successfully
    context->error_code = ErrorCode::kSuccess;
    context->SetState(CloseState::Create());
  }

  bool IsTerminal(CopyContext* context) { return false; }
};

}  // copy
}  // services
}  // ssf

#endif  // SSF_SERVICES_COPY_STATE_SENDER_WAIT_INTEGRITY_CHECK_REPLY_STATE_H_

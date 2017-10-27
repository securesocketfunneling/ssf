#ifndef SSF_SERVICES_COPY_STATE_RECEIVER_WAIT_INTEGRITY_CHECK_STATE_H_
#define SSF_SERVICES_COPY_STATE_RECEIVER_WAIT_INTEGRITY_CHECK_STATE_H_

#include <iostream>

#include <msgpack.hpp>

#include <ssf/log/log.h>

#include "common/error/error.h"

#include "services/copy/i_copy_state.h"
#include "services/copy/packet/init.h"
#include "services/copy/state/on_abort.h"
#include "services/copy/state/receiver/abort_receiver_state.h"
#include "services/copy/state/receiver/send_integrity_check_reply_state.h"

namespace ssf {
namespace services {
namespace copy {

class WaitIntegrityCheckRequestState : ICopyState {
 public:
  template <typename... Args>
  static ICopyStateUPtr Create(Args&&... args) {
    return ICopyStateUPtr(
        new WaitIntegrityCheckRequestState(std::forward<Args>(args)...));
  }

 private:
  WaitIntegrityCheckRequestState() : ICopyState() {}

 public:
  // ICopyState
  void Enter(CopyContext* context, boost::system::error_code& ec) {
    SSF_LOG(kLogTrace)
        << "microservice[copy][wait_integrity_check_request] enter";
  }

  bool FillOutboundPacket(CopyContext* context, Packet* packet,
                          boost::system::error_code& ec) {
    return false;
  }

  void ProcessInboundPacket(CopyContext* context, const Packet& packet,
                            boost::system::error_code& ec) {
    if (packet.type() == PacketType::kAbort) {
      return OnReceiverAbortPacket(context, packet, ec);
    }

    if (packet.type() != PacketType::kCheckIntegrityRequest) {
      SSF_LOG(kLogDebug) << "microservice[copy][wait_integrity_check_request] "
                            "cannot process inbound packet";
      context->SetState(
          AbortReceiverState::Create(ErrorCode::kInboundPacketNotSupported));
      return;
    }

    boost::system::error_code convert_ec;
    CheckIntegrityRequest<CopyContext::Hash> req;
    PacketToPayload(packet, req, convert_ec);
    if (convert_ec) {
      SSF_LOG(kLogDebug) << "microservice[copy][wait_integrity_check_request] "
                            "cannot convert packet to integrity check request";
      context->SetState(AbortReceiverState::Create(
          ErrorCode::kIntegrityCheckRequestPacketCorrupted));
    }

    context->input_file_digest = req.input_file_digest;

    context->SetState(SendIntegrityCheckReplyState::Create());
  }

  bool IsTerminal(CopyContext* context) { return false; }
};

}  // copy
}  // services
}  // ssf

#endif  // SSF_SERVICES_COPY_STATE_RECEIVER_WAIT_INTEGRITY_CHECK_STATE_H_

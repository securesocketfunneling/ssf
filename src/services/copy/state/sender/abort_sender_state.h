#ifndef SSF_SERVICES_COPY_STATE_SENDER_ABORT_SENDER_STATE_H_
#define SSF_SERVICES_COPY_STATE_SENDER_ABORT_SENDER_STATE_H_

#include <ssf/log/log.h>

#include "common/error/error.h"

#include "services/copy/error_code.h"
#include "services/copy/i_copy_state.h"
#include "services/copy/state/on_abort.h"
#include "services/copy/state/sender/wait_abort_ack_state.h"

namespace ssf {
namespace services {
namespace copy {

class AbortSenderState : ICopyState {
 public:
  template <typename... Args>
  static ICopyStateUPtr Create(Args&&... args) {
    return ICopyStateUPtr(new AbortSenderState(std::forward<Args>(args)...));
  }

 private:
  AbortSenderState(ErrorCode error_code) : error_code_(error_code) {}

 public:
  // ICopyState
  void Enter(CopyContext* context, boost::system::error_code& ec) {
    SSF_LOG(kLogTrace) << "microservice[copy][abort_sender] enter";
  }

  bool FillOutboundPacket(CopyContext* context, Packet* packet,
                          boost::system::error_code& ec) override {
    // update context error code
    context->error_code = error_code_;

    SSF_LOG(kLogDebug) << "microservice[copy][abort_sender] send abort "
                       << error_code_;
    Abort abort(error_code_);

    PayloadToPacket(abort, packet, ec);
    if (ec) {
      SSF_LOG(kLogDebug)
          << "microservice[copy][abort_receiver] cannot fill outbound packet";
      return false;
    }

    context->SetState(WaitAbortAckState::Create());
    return true;
  }
  void ProcessInboundPacket(CopyContext* context, const Packet& packet,
                            boost::system::error_code& ec) override {
    if (packet.type() == PacketType::kAbort) {
      return OnSenderAbortPacket(context, packet, ec);
    }

    // noop
  }

  bool IsTerminal(CopyContext* context) override { return false; }

 private:
  ErrorCode error_code_;
};

}  // copy
}  // services
}  // ssf

#endif  // SSF_SERVICES_COPY_STATE_SENDER_ABORT_SENDER_STATE_H_

#ifndef SSF_SERVICES_COPY_STATE_RECEIVER_ABORT_RECEIVER_STATE_H_
#define SSF_SERVICES_COPY_STATE_RECEIVER_ABORT_RECEIVER_STATE_H_

#include <ssf/log/log.h>

#include "common/error/error.h"

#include "services/copy/i_copy_state.h"
#include "services/copy/packet/error.h"
#include "services/copy/packet_helper.h"
#include "services/copy/state/receiver/wait_close_state.h"

namespace ssf {
namespace services {
namespace copy {

class AbortReceiverState : ICopyState {
 public:
  template <typename... Args>
  static ICopyStateUPtr Create(Args&&... args) {
    return ICopyStateUPtr(new AbortReceiverState(std::forward<Args>(args)...));
  }

 private:
  AbortReceiverState(ErrorCode error_code) : error_code_(error_code) {}

 public:
  // ICopyState
  void Enter(CopyContext* context, boost::system::error_code& ec) {
    SSF_LOG(kLogDebug) << "microservice[copy][abort_receiver] enter";
  }

  bool FillOutboundPacket(CopyContext* context, Packet* packet,
                          boost::system::error_code& ec) override {
    // update context error code
    context->error_code = error_code_;

    SSF_LOG(kLogDebug) << "microservice[copy][abort_receiver] send abort "
                       << error_code_;
    Abort abort(error_code_);

    PayloadToPacket(abort, packet, ec);
    if (ec) {
      SSF_LOG(kLogDebug)
          << "microservice[copy][abort_receiver] cannot fill outbound packet";
      return false;
    }

    context->SetState(WaitCloseState::Create());

    return true;
  }
  void ProcessInboundPacket(CopyContext* context, const Packet& packet,
                            boost::system::error_code& ec) override {
    if (packet.type() == PacketType::kAbort) {
      return OnReceiverAbortPacket(context, packet, ec);
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

#endif  // SSF_SERVICES_COPY_STATE_RECEIVER_ABORT_RECEIVER_STATE_H_

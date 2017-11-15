#ifndef SSF_SERVICES_COPY_STATE_SENDER_WAIT_ABORT_ACK_STATE_H_
#define SSF_SERVICES_COPY_STATE_SENDER_WAIT_ABORT_ACK_STATE_H_

#include <ssf/log/log.h>

#include "common/error/error.h"

#include "services/copy/i_copy_state.h"
#include "services/copy/state/sender/close_state.h"

namespace ssf {
namespace services {
namespace copy {

class WaitAbortAckState : ICopyState {
 public:
  template <typename... Args>
  static ICopyStateUPtr Create(Args&&... args) {
    return ICopyStateUPtr(new WaitAbortAckState(std::forward<Args>(args)...));
  }

 public:
  // ICopyState
  void Enter(CopyContext* context, boost::system::error_code& ec) override {
    SSF_LOG("microservice", trace, "[copy][wait_abort_ack] enter");
  }

  bool FillOutboundPacket(CopyContext* context, Packet* packet,
                          boost::system::error_code& ec) override {
    return false;
  }

  void ProcessInboundPacket(CopyContext* context, const Packet& packet,
                            boost::system::error_code& ec) override {
    if (packet.type() != PacketType::kAbortAck) {
      SSF_LOG("microservice", debug,
              "[copy][wait_abort_ack] cannot process inbound packet");
      ec.assign(::error::protocol_error, ::error::get_ssf_category());
      return;
    }

    context->SetState(CloseState::Create());
  }

  bool IsTerminal(CopyContext* context) override { return false; }
};

}  // copy
}  // services
}  // ssf

#endif  // SSF_SERVICES_COPY_STATE_SENDER_WAIT_ABORT_ACK_STATE_H_

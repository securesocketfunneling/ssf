#ifndef SSF_SERVICES_COPY_STATE_RECEIVER_SEND_EOF_STATE_H_
#define SSF_SERVICES_COPY_STATE_RECEIVER_SEND_EOF_STATE_H_

#include <iostream>

#include <msgpack.hpp>

#include <ssf/log/log.h>

#include "common/error/error.h"

#include "services/copy/i_copy_state.h"
#include "services/copy/packet/error.h"
#include "services/copy/packet_helper.h"
#include "services/copy/state/receiver/wait_close_state.h"

namespace ssf {
namespace services {
namespace copy {

class SendAbortAckState : ICopyState {
 public:
  template <typename... Args>
  static ICopyStateUPtr Create(Args&&... args) {
    return ICopyStateUPtr(new SendAbortAckState(std::forward<Args>(args)...));
  }

 private:
  SendAbortAckState() : ICopyState() {}

 public:
  // ICopyState
  void Enter(CopyContext* context, boost::system::error_code& ec) {
    SSF_LOG("microservice", trace, "[copy][send_abort_ack] enter");
  }

  bool FillOutboundPacket(CopyContext* context, Packet* packet,
                          boost::system::error_code& ec) {
    AbortAck ack(context->error_code);

    PayloadToPacket(ack, packet, ec);
    if (ec) {
      SSF_LOG("microservice", debug,
              "[copy][send_abort_ack] cannot convert abort ack to packet");
      return false;
    }

    context->SetState(WaitCloseState::Create());

    return true;
  }

  void ProcessInboundPacket(CopyContext* context, const Packet& packet,
                            boost::system::error_code& ec) {
    if (packet.type() == PacketType::kAbort) {
      return OnReceiverAbortPacket(context, packet, ec);
    }
  }

  bool IsTerminal(CopyContext* context) { return false; }
};

}  // copy
}  // services
}  // ssf

#endif  // SSF_SERVICES_COPY_STATE_RECEIVER_SEND_EOF_STATE_H_

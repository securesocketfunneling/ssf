#ifndef SSF_SERVICES_COPY_STATE_RECEIVER_SEND_EOF_STATE_H_
#define SSF_SERVICES_COPY_STATE_RECEIVER_SEND_EOF_STATE_H_

#include <iostream>

#include <msgpack.hpp>

#include <ssf/log/log.h>

#include "common/error/error.h"

#include "services/copy/i_copy_state.h"
#include "services/copy/state/on_abort.h"
#include "services/copy/state/receiver/abort_receiver_state.h"
#include "services/copy/state/receiver/wait_close_state.h"
#include "services/copy/state/receiver/wait_integrity_check_request_state.h"

namespace ssf {
namespace services {
namespace copy {

class SendEofState : ICopyState {
 public:
  template <typename... Args>
  static ICopyStateUPtr Create(Args&&... args) {
    return ICopyStateUPtr(new SendEofState(std::forward<Args>(args)...));
  }

 private:
  SendEofState() : ICopyState() {}

 public:
  // ICopyState
  void Enter(CopyContext* context, boost::system::error_code& ec) {
    SSF_LOG("microservice", trace, "[copy][send_eof] enter");
  }

  bool FillOutboundPacket(CopyContext* context, Packet* packet,
                          boost::system::error_code& ec) {
    packet->set_type(PacketType::kEof);
    packet->set_payload_size(0);

    if (context->check_file_integrity && !context->is_stdin_input) {
      context->SetState(WaitIntegrityCheckRequestState::Create());
    } else {
      context->error_code = ErrorCode::kSuccess;
      context->SetState(WaitCloseState::Create());
    }

    return true;
  }

  void ProcessInboundPacket(CopyContext* context, const Packet& packet,
                            boost::system::error_code& ec) {
    if (packet.type() == PacketType::kAbort) {
      return OnReceiverAbortPacket(context, packet, ec);
    }

    SSF_LOG("microservice", debug,
            "[copy][send_eof] cannot process inbound packet");
    context->SetState(
        AbortReceiverState::Create(ErrorCode::kInboundPacketNotSupported));
  }

  bool IsTerminal(CopyContext* context) { return false; }
};

}  // copy
}  // services
}  // ssf

#endif  // SSF_SERVICES_COPY_STATE_RECEIVER_SEND_EOF_STATE_H_

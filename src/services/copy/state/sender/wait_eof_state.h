#ifndef SSF_SERVICES_COPY_STATE_SENDER_WAIT_EOF_STATE_H_
#define SSF_SERVICES_COPY_STATE_SENDER_WAIT_EOF_STATE_H_

#include <iostream>

#include <msgpack.hpp>

#include <ssf/log/log.h>

#include "common/error/error.h"

#include "services/copy/i_copy_state.h"
#include "services/copy/packet/init.h"
#include "services/copy/state/on_abort.h"
#include "services/copy/state/sender/abort_sender_state.h"
#include "services/copy/state/sender/close_state.h"
#include "services/copy/state/sender/send_integrity_check_request_state.h"

namespace ssf {
namespace services {
namespace copy {

class WaitEofState : ICopyState {
 public:
  template <typename... Args>
  static ICopyStateUPtr Create(Args&&... args) {
    return ICopyStateUPtr(new WaitEofState(std::forward<Args>(args)...));
  }

 private:
  WaitEofState() : ICopyState() {}

 public:
  // ICopyState
  void Enter(CopyContext* context, boost::system::error_code& ec) {
    SSF_LOG("microservice", trace, "[copy][wait_eof] enter");
  }

  bool FillOutboundPacket(CopyContext* context, Packet* packet,
                          boost::system::error_code& ec) {
    return false;
  }

  void ProcessInboundPacket(CopyContext* context, const Packet& packet,
                            boost::system::error_code& ec) {
    if (context->input.good() && context->input.is_open()) {
      context->input.close();
    }

    if (packet.type() == PacketType::kAbort) {
      return OnSenderAbortPacket(context, packet, ec);
    }

    if (packet.type() != PacketType::kEof) {
      SSF_LOG("microservice", debug,
              "[copy][wait_eof] cannot process inbound packet");
      context->SetState(
          AbortSenderState::Create(ErrorCode::kInboundPacketNotSupported));
      return;
    }

    if (context->check_file_integrity && !context->is_stdin_input) {
      context->SetState(SendIntegrityCheckRequestState::Create());
    } else {
      context->error_code = ErrorCode::kSuccess;
      context->SetState(CloseState::Create());
    }
  }

  bool IsTerminal(CopyContext* context) { return false; }
};

}  // copy
}  // services
}  // ssf

#endif  // SSF_SERVICES_COPY_STATE_SENDER_WAIT_EOF_STATE_H_

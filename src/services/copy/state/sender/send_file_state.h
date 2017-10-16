#ifndef SSF_SERVICES_COPY_STATE_SENDER_SEND_FILE_STATE_H_
#define SSF_SERVICES_COPY_STATE_SENDER_SEND_FILE_STATE_H_

#include <iostream>

#include <msgpack.hpp>

#include <ssf/log/log.h>

#include "common/error/error.h"

#include "services/copy/i_copy_state.h"
#include "services/copy/packet/init.h"
#include "services/copy/state/on_abort.h"
#include "services/copy/state/sender/abort_sender_state.h"
#include "services/copy/state/sender/wait_eof_state.h"

namespace ssf {
namespace services {
namespace copy {

class SendFileState : ICopyState {
 public:
  template <typename... Args>
  static ICopyStateUPtr Create(Args&&... args) {
    return ICopyStateUPtr(new SendFileState(std::forward<Args>(args)...));
  }

 private:
  SendFileState() : ICopyState() {}

 public:
  // ICopyState
  void Enter(CopyContext* context, boost::system::error_code& ec) {
    SSF_LOG(kLogDebug) << "microservice[copy][send_file] enter";
  }

  bool FillOutboundPacket(CopyContext* context, Packet* packet,
                          boost::system::error_code& ec) {
    auto& input = context->is_stdin_input ? std::cin : context->input;

    if (input.good()) {
      try {
        input.read(packet->buffer().data(), packet->buffer().size());
      } catch (const std::exception&) {
        SSF_LOG(kLogDebug)
            << "microservice[copy][send_file] error while reading input file";
        context->SetState(
            AbortSenderState::Create(ErrorCode::kInputFileReadError));
        return false;
      }
      packet->set_type(PacketType::kData);
      packet->set_payload_size(static_cast<uint32_t>(input.gcount()));
      return true;
    } else if (input.eof()) {
      packet->set_type(PacketType::kEof);
      packet->set_payload_size(0);
      context->SetState(WaitEofState::Create());
      return true;
    } else {
      SSF_LOG(kLogDebug)
          << "microservice[copy][send_file] cannot read input file";
      context->SetState(
          AbortSenderState::Create(ErrorCode::kInputFileReadError));
      return false;
    }
  }

  void ProcessInboundPacket(CopyContext* context, const Packet& packet,
                            boost::system::error_code& ec) {
    if (packet.type() == PacketType::kAbort) {
      return OnSenderAbortPacket(context, packet, ec);
    }

    SSF_LOG(kLogDebug)
        << "microservice[copy][send_file] cannot process inbound packet";
    context->SetState(
        AbortSenderState::Create(ErrorCode::kInboundPacketNotSupported));
  }

  bool IsTerminal(CopyContext* context) { return false; }
};

}  // copy
}  // services
}  // ssf

#endif  // SSF_SERVICES_COPY_STATE_SENDER_SEND_FILE_STATE_H_

#ifndef SSF_SERVICES_COPY_STATE_SENDER_SEND_INIT_REQUEST_STATE_H_
#define SSF_SERVICES_COPY_STATE_SENDER_SEND_INIT_REQUEST_STATE_H_

#include <iostream>
#include <msgpack.hpp>

#include <ssf/log/log.h>

#include "common/error/error.h"

#include "services/copy/i_copy_state.h"
#include "services/copy/packet_helper.h"
#include "services/copy/state/on_abort.h"
#include "services/copy/state/sender/abort_sender_state.h"
#include "services/copy/state/sender/wait_init_reply_state.h"

namespace ssf {
namespace services {
namespace copy {

class SendInitRequestState : ICopyState {
 public:
  template <typename... Args>
  static ICopyStateUPtr Create(Args&&... args) {
    return ICopyStateUPtr(
        new SendInitRequestState(std::forward<Args>(args)...));
  }

 private:
  SendInitRequestState() : ICopyState() {}

 public:
  // ICopyState
  void Enter(CopyContext* context, boost::system::error_code& ec) {
    SSF_LOG(kLogTrace) << "microservice[copy][send_init_request] enter";
  }

  bool FillOutboundPacket(CopyContext* context, Packet* packet,
                          boost::system::error_code& ec) override {
    // create and sent Init packet
    InitRequest req(context->input_filepath, context->check_file_integrity,
                    context->is_stdin_input, context->resume, context->filesize,
                    context->output_dir, context->output_filename);

    boost::system::error_code convert_ec;
    PayloadToPacket(req, packet, convert_ec);
    if (convert_ec) {
      SSF_LOG(kLogDebug) << "microservice[copy][send_init_request] cannot "
                            "convert init request to packet";
      context->SetState(
          AbortSenderState::Create(ErrorCode::kInitRequestPacketNotGenerated));
      return false;
    }

    context->SetState(WaitInitReplyState::Create());

    return true;
  }

  void ProcessInboundPacket(CopyContext* context, const Packet& packet,
                            boost::system::error_code& ec) override {
    if (packet.type() == PacketType::kAbort) {
      return OnSenderAbortPacket(context, packet, ec);
    }

    SSF_LOG(kLogDebug)
        << "microservice[copy][send_init_request] cannot process packet type";
    context->SetState(
        AbortSenderState::Create(ErrorCode::kInboundPacketNotSupported));
  }

  bool IsTerminal(CopyContext* context) override { return false; }
};

}  // copy
}  // services
}  // ssf

#endif  // SSF_SERVICES_COPY_STATE_SENDER_SEND_INIT_REQUEST_STATE_H_

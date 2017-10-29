#ifndef SSF_SERVICES_COPY_STATE_RECEIVER_SEND_INIT_REPLY_STATE_H_
#define SSF_SERVICES_COPY_STATE_RECEIVER_SEND_INIT_REPLY_STATE_H_

#include <iostream>
#include <msgpack.hpp>

#include <ssf/log/log.h>

#include "common/crypto/hash.h"
#include "common/error/error.h"

#include "services/copy/i_copy_state.h"
#include "services/copy/state/on_abort.h"
#include "services/copy/state/receiver/abort_receiver_state.h"
#include "services/copy/state/receiver/receive_file_state.h"

namespace ssf {
namespace services {
namespace copy {

class SendInitReplyState : ICopyState {
 public:
  template <typename... Args>
  static ICopyStateUPtr Create(Args&&... args) {
    return ICopyStateUPtr(new SendInitReplyState(std::forward<Args>(args)...));
  }

 private:
  SendInitReplyState() : ICopyState() {}

 public:
  // ICopyState
  void Enter(CopyContext* context, boost::system::error_code& ec) {
    SSF_LOG("microservice", trace, "[copy][send_init_reply] enter");
  }

  bool FillOutboundPacket(CopyContext* context, Packet* packet,
                          boost::system::error_code& ec) override {
    InitRequest req(context->input_filepath, context->check_file_integrity,
                    context->is_stdin_input, context->resume, context->filesize,
                    context->output_dir, context->output_filename);

    auto& output_fh = context->output;
    InitReply::Hash::Digest file_digest = {{0}};
    if (context->resume) {
      output_fh.seekp(0, std::ofstream::end);
      SSF_LOG("microservice", debug,
              "[copy][send_init_reply] resume file "
              "transfer at byte index {}",
              output_fh.tellp());

      boost::system::error_code hash_ec;
      file_digest = ssf::crypto::HashFile<InitReply::Hash>(
          context->GetOutputFilepath(), hash_ec);
      if (!hash_ec) {
        context->start_offset = output_fh.tellp();
      } else {
        SSF_LOG("microservice", debug,
                "[copy][send_init_reply] could not "
                "generate digest for "
                "output_file. Do not resume file copy");
        context->start_offset = 0;
      }
    }

    InitReply rep(req, context->start_offset, file_digest,
                  InitReply::Status::kInitializationSucceeded);

    boost::system::error_code convert_ec;
    PayloadToPacket(rep, packet, convert_ec);
    if (convert_ec) {
      SSF_LOG("microservice", debug,
              "[copy][send_init_reply] cannot "
              "convert init reply to packet");
      context->SetState(
          AbortReceiverState::Create(ErrorCode::kInitReplyPacketNotGenerated));
      return false;
    }

    context->SetState(ReceiveFileState::Create());
    return true;
  }

  void ProcessInboundPacket(CopyContext* context, const Packet& packet,
                            boost::system::error_code& ec) override {
    if (packet.type() == PacketType::kAbort) {
      return OnReceiverAbortPacket(context, packet, ec);
    }

    context->SetState(
        AbortReceiverState::Create(ErrorCode::kInboundPacketNotSupported));
    return;
  }

  bool IsTerminal(CopyContext* context) override { return false; }
};

}  // copy
}  // services
}  // ssf

#endif  // SSF_SERVICES_COPY_STATE_RECEIVER_SEND_INIT_REPLY_STATE_H_

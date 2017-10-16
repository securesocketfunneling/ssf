#ifndef SSF_SERVICES_COPY_STATE_SENDER_WAIT_INIT_REPLY_STATE_H_
#define SSF_SERVICES_COPY_STATE_SENDER_WAIT_INIT_REPLY_STATE_H_

#include <iostream>
#include <msgpack.hpp>

#include <ssf/log/log.h>

#include "common/crypto/hash.h"
#include "common/error/error.h"

#include "services/copy/i_copy_state.h"
#include "services/copy/packet/check.h"
#include "services/copy/state/on_abort.h"
#include "services/copy/state/sender/abort_sender_state.h"
#include "services/copy/state/sender/send_file_state.h"

namespace ssf {
namespace services {
namespace copy {

class WaitInitReplyState : ICopyState {
 public:
  template <typename... Args>
  static ICopyStateUPtr Create(Args&&... args) {
    return ICopyStateUPtr(new WaitInitReplyState(std::forward<Args>(args)...));
  }

 private:
  WaitInitReplyState() : ICopyState() {}

 public:
  // ICopyState
  void Enter(CopyContext* context, boost::system::error_code& ec) {
    SSF_LOG(kLogDebug) << "microservice[copy][wait_init_reply] enter";
  }

  bool FillOutboundPacket(CopyContext* context, Packet* packet,
                          boost::system::error_code& ec) override {
    return false;
  }

  void ProcessInboundPacket(CopyContext* context, const Packet& packet,
                            boost::system::error_code& ec) override {
    if (packet.type() == PacketType::kAbort) {
      return OnSenderAbortPacket(context, packet, ec);
    }

    if (packet.type() != PacketType::kInitReply) {
      SSF_LOG(kLogDebug)
          << "microservice[copy][wait_init_reply] cannot process packet type";
      context->SetState(
          AbortSenderState::Create(ErrorCode::kInboundPacketNotSupported));
      return;
    }

    // create InitReply struct from payload
    InitReply init_rep;

    boost::system::error_code convert_ec;
    PacketToPayload(packet, init_rep, convert_ec);
    if (convert_ec) {
      SSF_LOG(kLogDebug) << "microservice[copy][wait_init_reply] cannot "
                            "convert packet to init reply";
      context->SetState(
          AbortSenderState::Create(ErrorCode::kInitReplyPacketCorrupted));
      return;
    }

    if (init_rep.status != InitReply::Status::kInitializationSucceeded) {
      SSF_LOG(kLogDebug)
          << "microservice[copy][wait_init_reply] remote initialization failed";
      context->SetState(
          AbortSenderState::Create(ErrorCode::kCopyInitializationFailed));
      return;
    }

    // update context from receiver
    // if resume requested, check file base integrity before sending file
    context->start_offset = 0;
    if (init_rep.req.resume && init_rep.start_offset > 0) {
      boost::system::error_code hash_ec;
      auto digest = ssf::crypto::HashFile<CopyContext::Hash>(
          context->input_filepath, init_rep.start_offset, hash_ec);
      if (hash_ec) {
        SSF_LOG(kLogDebug) << "microservice[copy][wait_init_reply] cannot "
                              "generate input file hash";
        context->SetState(
            AbortSenderState::Create(ErrorCode::kInputFileDigestNotAvailable));
        return;
      }

      if (!std::equal(digest.begin(), digest.end(),
                      init_rep.current_filehash.begin())) {
        SSF_LOG(kLogDebug)
            << "microservice[copy][wait_init_reply] input file and output "
               "file are different";
        context->SetState(AbortSenderState::Create(
            ErrorCode::kResumeFileTransferNotPermitted));
        return;
      }
      context->start_offset = init_rep.start_offset;
    }

    auto& input_file = context->input;
    if (!context->is_stdin_input) {
      input_file.open(context->input_filepath,
                      std::ifstream::binary | std::ifstream::in);
      if (!input_file.is_open() || !input_file.good()) {
        SSF_LOG(kLogDebug)
            << "microservice[copy][wait_init_reply] cannot open input file "
            << context->input_filepath;
        context->SetState(
            AbortSenderState::Create(ErrorCode::kInputFileNotAvailable));
        return;
      }
      input_file.seekg(context->start_offset, std::ifstream::beg);
    }

    context->SetState(SendFileState::Create());
  }

  bool IsTerminal(CopyContext* context) override { return false; }
};

}  // copy
}  // services
}  // ssf

#endif  // SSF_SERVICES_COPY_STATE_SENDER_WAIT_INIT_REPLY_STATE_H_

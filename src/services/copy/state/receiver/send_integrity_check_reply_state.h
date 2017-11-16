#ifndef SSF_SERVICES_COPY_STATE_SEND_INTEGRITY_CHECK_REPLY_STATE_H_
#define SSF_SERVICES_COPY_STATE_SEND_INTEGRITY_CHECK_REPLY_STATE_H_

#include <iostream>

#include <msgpack.hpp>

#include <ssf/log/log.h>

#include "common/error/error.h"

#include "common/crypto/hash.h"
#include "services/copy/i_copy_state.h"
#include "services/copy/packet/check.h"
#include "services/copy/packet_helper.h"
#include "services/copy/state/on_abort.h"
#include "services/copy/state/receiver/abort_receiver_state.h"
#include "services/copy/state/receiver/wait_close_state.h"

namespace ssf {
namespace services {
namespace copy {

class SendIntegrityCheckReplyState : ICopyState {
 public:
  template <typename... Args>
  static ICopyStateUPtr Create(Args&&... args) {
    return ICopyStateUPtr(
        new SendIntegrityCheckReplyState(std::forward<Args>(args)...));
  }

 private:
  SendIntegrityCheckReplyState() : ICopyState() {}

 public:
  // ICopyState
  void Enter(CopyContext* context, boost::system::error_code& ec) {
    SSF_LOG("microservice", trace, "[copy][send_integrity_check_reply] enter");
  }

  bool FillOutboundPacket(CopyContext* context, Packet* packet,
                          boost::system::error_code& ec) {
    boost::system::error_code hash_ec;
    context->output_file_digest = ssf::crypto::HashFile<CopyContext::Hash>(
        context->GetOutputFilepath().GetString(), hash_ec);

    CheckIntegrityRequest<CopyContext::Hash> req(context->input_file_digest);

    bool integrity_checked = true;
    if (!hash_ec) {
      integrity_checked = std::equal(context->input_file_digest.begin(),
                                     context->input_file_digest.end(),
                                     context->output_file_digest.begin());
    }

    if (!integrity_checked) {
      SSF_LOG("microservice", debug,
              "[copy][send_integrity_check_reply] "
              "output file is corrupted");
      // remove file
      boost::system::error_code remove_ec;
      context->fs.Remove(context->GetOutputFilepath(), remove_ec);
      if (remove_ec) {
        SSF_LOG("microservice", debug,
                "[copy][send_integrity_check_reply] "
                "could not remove output file {}",
                remove_ec.message());
      }
    }

    CheckIntegrityReply<CopyContext::Hash> rep(
        req, context->output_file_digest,
        (!hash_ec && integrity_checked)
            ? CheckIntegrityStatus::kCheckIntegritySucceeded
            : CheckIntegrityStatus::kCheckIntegrityFailed);

    boost::system::error_code convert_ec;
    PayloadToPacket(rep, packet, convert_ec);
    if (convert_ec) {
      SSF_LOG("microservice", debug,
              "[copy][send_integrity_check_reply] "
              "cannot convert init reply to packet");
      context->SetState(AbortReceiverState::Create(
          ErrorCode::kIntegrityCheckReplyPacketNotGenerated));
      return false;
    }

    context->error_code = ErrorCode::kSuccess;
    context->SetState(WaitCloseState::Create());
    return true;
  }

  void ProcessInboundPacket(CopyContext* context, const Packet& packet,
                            boost::system::error_code& ec) {
    if (packet.type() == PacketType::kAbort) {
      return OnReceiverAbortPacket(context, packet, ec);
    }

    SSF_LOG("microservice", debug,
            "[copy][send_integrity_check_reply] "
            "cannot process inbound packet");
    context->SetState(
        AbortReceiverState::Create(ErrorCode::kInboundPacketNotSupported));
  }

  bool IsTerminal(CopyContext* context) { return false; }
};

}  // copy
}  // services
}  // ssf

#endif  // SSF_SERVICES_COPY_STATE_SEND_INTEGRITY_CHECK_REPLY_STATE_H_

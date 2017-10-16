#include "services/copy/state/on_abort.h"

#include <ssf/log/log.h>

#include "services/copy/copy_context.h"
#include "services/copy/packet.h"
#include "services/copy/packet/error.h"
#include "services/copy/packet_helper.h"
#include "services/copy/state/receiver/send_abort_ack_state.h"
#include "services/copy/state/sender/close_state.h"

namespace ssf {
namespace services {
namespace copy {

void OnReceiverAbortPacket(CopyContext* context, const Packet& packet,
                           boost::system::error_code& ec) {
  Abort abort;
  PacketToPayload(packet, abort, ec);
  if (ec) {
    SSF_LOG(kLogError) << "microservice[copy][on_receiver_abort] cannot "
                          "convert packet to abort message";
    return;
  }
  context->error_code = abort.error_code;

  context->SetState(SendAbortAckState::Create());
}

void OnSenderAbortPacket(CopyContext* context, const Packet& packet,
                         boost::system::error_code& ec) {
  Abort abort;
  PacketToPayload(packet, abort, ec);
  if (ec) {
    SSF_LOG(kLogError) << "microservice[copy][on_sender_abort] cannot "
                          "convert packet to abort message";
    return;
  }
  context->error_code = abort.error_code;

  context->SetState(CloseState::Create());
}

}  // copy
}  // services
}  // ssf
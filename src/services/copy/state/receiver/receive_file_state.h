#ifndef SSF_SERVICES_COPY_STATE_RECEIVER_RECEIVE_FILE_STATE_H_
#define SSF_SERVICES_COPY_STATE_RECEIVER_RECEIVE_FILE_STATE_H_

#include <msgpack.hpp>

#include <ssf/log/log.h>

#include "common/error/error.h"

#include "services/copy/i_copy_state.h"
#include "services/copy/state/on_abort.h"
#include "services/copy/state/receiver/abort_receiver_state.h"
#include "services/copy/state/receiver/send_eof_state.h"

namespace ssf {
namespace services {
namespace copy {

class ReceiveFileState : ICopyState {
 public:
  template <typename... Args>
  static ICopyStateUPtr Create(Args&&... args) {
    return ICopyStateUPtr(new ReceiveFileState(std::forward<Args>(args)...));
  }

 private:
  ReceiveFileState() : ICopyState() {}

 public:
  // ICopyState
  void Enter(CopyContext* context, boost::system::error_code& ec) {
    SSF_LOG(kLogDebug) << "microservice[copy][receive_file] enter";
  }

  bool FillOutboundPacket(CopyContext* context, Packet* packet,
                          boost::system::error_code& ec) {
    return false;
  }

  void ProcessInboundPacket(CopyContext* context, const Packet& packet,
                            boost::system::error_code& ec) {
    switch (packet.type()) {
      case PacketType::kEof: {
        SSF_LOG(kLogDebug) << "microservice[copy][receive_file] eof";
        context->output.close();

        context->SetState(SendEofState::Create());
        break;
      }
      case PacketType::kData: {
        try {
          // write data into output file
          context->output.write(packet.buffer().data(), packet.payload_size());
        } catch (const std::exception&) {
          SSF_LOG(kLogDebug) << "microservice[copy][receive_file] error while "
                                "writing to output file";
          context->SetState(
              AbortReceiverState::Create(ErrorCode::kOutputFileWriteError));
          return;
        }
        if (!context->output.good()) {
          SSF_LOG(kLogDebug) << "microservice[copy][receive_file] write failed";
          context->SetState(
              AbortReceiverState::Create(ErrorCode::kOutputFileWriteError));
          return;
        }
        break;
      }
      case PacketType::kAbort: {
        return OnReceiverAbortPacket(context, packet, ec);
      }
      default: {
        SSF_LOG(kLogDebug) << "microservice[copy][receive_file] cannot "
                              "process inbound packet";
        context->SetState(
            AbortReceiverState::Create(ErrorCode::kInboundPacketNotSupported));
        return;
      }
    };
  }

  bool IsTerminal(CopyContext* context) { return false; }
};

}  // copy
}  // services
}  // ssf

#endif  // SSF_SERVICES_COPY_STATE_RECEIVER_RECEIVE_FILE_STATE_H_

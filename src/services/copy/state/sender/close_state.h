#ifndef SSF_SERVICES_COPY_STATE_SENDER_CLOSE_STATE_H_
#define SSF_SERVICES_COPY_STATE_SENDER_CLOSE_STATE_H_

#include <ssf/log/log.h>

#include "common/error/error.h"

#include "services/copy/i_copy_state.h"

namespace ssf {
namespace services {
namespace copy {

class CloseState : ICopyState {
 public:
  template <typename... Args>
  static ICopyStateUPtr Create(Args&&... args) {
    return ICopyStateUPtr(new CloseState(std::forward<Args>(args)...));
  }

 public:
  // ICopyState
  void Enter(CopyContext* context, boost::system::error_code& ec) {
    SSF_LOG("microservice", trace, "[copy][close] enter");
  }

  bool FillOutboundPacket(CopyContext* context, Packet* packet,
                          boost::system::error_code& ec) override {
    return false;
  }
  void ProcessInboundPacket(CopyContext* context, const Packet& packet,
                            boost::system::error_code& ec) override {
    // noop
  }

  bool IsClosed(CopyContext* context) override { return true; }

  bool IsTerminal(CopyContext* context) override { return true; }
};

}  // copy
}  // services
}  // ssf

#endif  // SSF_SERVICES_COPY_STATE_SENDER_CLOSE_STATE_H_

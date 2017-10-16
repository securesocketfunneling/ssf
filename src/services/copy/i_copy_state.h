#ifndef SSF_SERVICES_COPY_I_COPY_STATE_H_
#define SSF_SERVICES_COPY_I_COPY_STATE_H_

#include <memory>

#include <boost/system/error_code.hpp>

#include "common/error/error.h"

#include "services/copy/packet.h"

namespace ssf {
namespace services {
namespace copy {

class CopyContext;

class ICopyState {
 public:
  virtual ~ICopyState() {}

  virtual void Enter(CopyContext* context, boost::system::error_code& ec) {}

  virtual void Exit(CopyContext* context, boost::system::error_code& ec) {}

  virtual bool IsClosed(CopyContext* context) { return false; }

  virtual bool FillOutboundPacket(CopyContext* context, Packet* packet,
                                  boost::system::error_code& ec) = 0;

  virtual void ProcessInboundPacket(CopyContext* context, const Packet& packet,
                                    boost::system::error_code& ec) = 0;

  virtual bool IsTerminal(CopyContext* context) = 0;
};

using ICopyStateUPtr = std::unique_ptr<ICopyState>;

}  // copy
}  // services
}  // ssf

#endif  // SSF_SERVICES_COPY_I_COPY_STATE_H_

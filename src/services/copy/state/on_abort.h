#ifndef SSF_SERVICES_COPY_STATE_ON_ABORT_H_
#define SSF_SERVICES_COPY_STATE_ON_ABORT_H_

#include <boost/system/error_code.hpp>

namespace ssf {
namespace services {
namespace copy {

class CopyContext;
class Packet;

void OnReceiverAbortPacket(CopyContext* context, const Packet& packet,
                           boost::system::error_code& ec);

void OnSenderAbortPacket(CopyContext* context, const Packet& packet,
                         boost::system::error_code& ec);

}  // copy
}  // services
}  // ssf

#endif  // SSF_SERVICES_COPY_STATE_ON_ABORT_H_

#ifndef SSF_SERVICES_COPY_PACKET_ERROR_H_
#define SSF_SERVICES_COPY_PACKET_ERROR_H_

#include <string>

#include <msgpack.hpp>

#include "common/error/error.h"
#include "services/copy/packet.h"
#include "services/copy/packet/error_code.h"

namespace ssf {
namespace services {
namespace copy {

struct Abort {
  static const PacketType kType = PacketType::kAbort;

  Abort() : error_code(ErrorCode::kUnknown) {}
  Abort(ErrorCode i_error_code) : error_code(i_error_code) {}

  ErrorCode error_code;

  MSGPACK_DEFINE(error_code)
};

struct AbortAck {
  static const PacketType kType = PacketType::kAbortAck;

  AbortAck() : error_code(ErrorCode::kUnknown) {}
  AbortAck(ErrorCode i_error_code) : error_code(i_error_code) {}

  ErrorCode error_code;

  MSGPACK_DEFINE(error_code)
};

}  // copy
}  // services
}  // ssf

#endif  // SSF_SERVICES_COPY_PACKET_ERROR_H_
#ifndef SSF_SERVICES_COPY_PACKET_ERROR_CODE_H_
#define SSF_SERVICES_COPY_PACKET_ERROR_CODE_H_

#include <msgpack.hpp>

#include "services/copy/error_code.h"

MSGPACK_ADD_ENUM(ssf::services::copy::ErrorCode);

#endif  // SSF_SERVICES_COPY_PACKET_ERROR_CODE_H_
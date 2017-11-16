#ifndef SSF_SERVICES_COPY_PACKET_CHECK_H_
#define SSF_SERVICES_COPY_PACKET_CHECK_H_

#include <string>

#include <msgpack.hpp>

#include "common/crypto/sha1.h"
#include "services/copy/packet.h"

namespace ssf {
namespace services {
namespace copy {

enum CheckIntegrityStatus {
  kCheckIntegrityFailed = 0,
  kCheckIntegritySucceeded
};

template <class Hash>
struct CheckIntegrityRequest {
  static const PacketType kType = PacketType::kCheckIntegrityRequest;
  using Digest = typename Hash::Digest;

  CheckIntegrityRequest() : input_file_digest({{0}}) {}
  CheckIntegrityRequest(const Digest& i_input_file_digest)
      : input_file_digest(i_input_file_digest) {}

  Digest input_file_digest;

  MSGPACK_DEFINE(input_file_digest)
};

template <class Hash>
struct CheckIntegrityReply {
  static const PacketType kType = PacketType::kCheckIntegrityReply;
  using Digest = typename Hash::Digest;

  CheckIntegrityReply()
      : req(), output_file_digest({{0}}), status(kCheckIntegrityFailed) {}

  CheckIntegrityReply(const CheckIntegrityRequest<Hash>& i_req,
                      const Digest& i_output_file_digest,
                      CheckIntegrityStatus i_status)
      : req(i_req),
        output_file_digest(i_output_file_digest),
        status(i_status) {}

  CheckIntegrityRequest<Hash> req;
  Digest output_file_digest;
  CheckIntegrityStatus status;

  MSGPACK_DEFINE(req, output_file_digest, status)
};

}  // copy
}  // services
}  // ssf

MSGPACK_ADD_ENUM(ssf::services::copy::CheckIntegrityStatus);

#endif  // SSF_SERVICES_COPY_PACKET_CHECK_H_

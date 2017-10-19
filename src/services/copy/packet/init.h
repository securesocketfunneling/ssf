#ifndef SSF_SERVICES_COPY_PACKET_INIT_H_
#define SSF_SERVICES_COPY_PACKET_INIT_H_

#include <cstdint>

#include <string>

#include <msgpack.hpp>

#include "common/crypto/sha1.h"
#include "services/copy/packet.h"

namespace ssf {
namespace services {
namespace copy {

struct InitRequest {
  static const PacketType kType = PacketType::kInitRequest;

  InitRequest() {}

  InitRequest(const std::string& i_input_filepath, bool i_check_file_integrity,
              bool i_stdin_input, bool i_resume, uint64_t i_filesize,
              const std::string& i_output_dir,
              const std::string& i_output_filename)
      : input_filepath(i_input_filepath),
        check_file_integrity(i_check_file_integrity),
        stdin_input(i_stdin_input),
        resume(i_resume),
        filesize(i_filesize),
        output_dir(i_output_dir),
        output_filename(i_output_filename) {}

  std::string input_filepath;
  bool check_file_integrity;
  bool stdin_input;
  bool resume;
  uint64_t filesize;
  std::string output_dir;
  std::string output_filename;

  MSGPACK_DEFINE(input_filepath, check_file_integrity, stdin_input, resume,
                 filesize, output_dir, output_filename)
};

struct InitReply {
  static const PacketType kType = PacketType::kInitReply;
  using Hash = ssf::crypto::Sha1;
  enum Status { kInitializationFailed = 0, kInitializationSucceeded };

  InitReply() : req(), status(kInitializationFailed) {}

  InitReply(const InitRequest& i_req, uint64_t i_start_offset,
            const ssf::crypto::Sha1::Digest& i_current_filehash,
            Status i_status)
      : req(i_req),
        start_offset(i_start_offset),
        current_filehash(i_current_filehash),
        status(i_status) {}

  InitRequest req;
  uint64_t start_offset;
  Hash::Digest current_filehash;
  Status status;

  MSGPACK_DEFINE(req, start_offset, current_filehash, status)
};

}  // copy
}  // services
}  // ssf

MSGPACK_ADD_ENUM(ssf::services::copy::InitReply::Status);

#endif  // SSF_SERVICES_COPY_PACKET_INIT_H_

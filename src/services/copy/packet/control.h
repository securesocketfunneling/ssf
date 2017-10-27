#ifndef SSF_SERVICES_COPY_PACKET_CONTROL_H_
#define SSF_SERVICES_COPY_PACKET_CONTROL_H_

#include <cstdint>

#include <string>

#include <msgpack.hpp>

#include "services/copy/packet.h"
#include "services/copy/packet/error_code.h"

namespace ssf {
namespace services {
namespace copy {

struct CopyRequest {
  static const PacketType kType = PacketType::kCopyRequest;

  CopyRequest()
      : is_from_stdin(false),
        is_resume(false),
        is_recursive(false),
        check_file_integrity(false),
        max_parallel_copies(1) {}

  CopyRequest(const CopyRequest& req)
      : is_from_stdin(req.is_from_stdin),
        is_resume(req.is_resume),
        is_recursive(req.is_recursive),
        check_file_integrity(req.check_file_integrity),
        max_parallel_copies(req.max_parallel_copies),
        input_pattern(req.input_pattern),
        output_pattern(req.output_pattern) {}

  CopyRequest(bool i_is_from_stdin, bool i_is_resume, bool i_is_recursive,
              bool i_check_file_integrity, uint32_t i_max_parallel_copies,
              const std::string& i_input_pattern,
              const std::string& i_output_pattern)
      : is_from_stdin(i_is_from_stdin),
        is_resume(i_is_resume),
        is_recursive(i_is_recursive),
        check_file_integrity(i_check_file_integrity),
        max_parallel_copies(i_max_parallel_copies),
        input_pattern(i_input_pattern),
        output_pattern(i_output_pattern) {}

  CopyRequest& operator=(const CopyRequest& other) {
    is_from_stdin = other.is_from_stdin;
    is_resume = other.is_resume;
    is_recursive = other.is_recursive;
    check_file_integrity = other.check_file_integrity;
    max_parallel_copies = other.max_parallel_copies;
    input_pattern = other.input_pattern;
    output_pattern = other.output_pattern;

    return *this;
  }

  bool is_from_stdin;
  bool is_resume;
  bool is_recursive;
  bool check_file_integrity;
  uint32_t max_parallel_copies;
  std::string input_pattern;
  std::string output_pattern;

  MSGPACK_DEFINE(is_from_stdin, is_resume, is_recursive, check_file_integrity,
                 max_parallel_copies, input_pattern, output_pattern)
};

struct CopyRequestAck {
  static const PacketType kType = PacketType::CopyRequestAck;
  enum Status { kRequestReceived, kRequestCorrupted };

  CopyRequestAck() : req(), status(kRequestCorrupted) {}

  CopyRequestAck(const CopyRequestAck& other)
      : req(other.req), status(other.status) {}

  CopyRequestAck(const CopyRequest& i_req, Status i_status)
      : req(i_req), status(i_status) {}

  CopyRequest req;
  Status status;

  MSGPACK_DEFINE(req, status)
};

struct CopyFinishedNotification {
  static const PacketType kType = PacketType::kCopyFinished;

  CopyFinishedNotification() : error_code(ErrorCode::kUnknown) {}

  CopyFinishedNotification(uint64_t i_files_count, uint64_t i_errors_count,
                           ErrorCode i_error_code)
      : files_count(i_files_count),
        errors_count(i_errors_count),
        error_code(i_error_code) {}

  uint64_t files_count;
  uint64_t errors_count;
  ErrorCode error_code;

  MSGPACK_DEFINE(files_count, errors_count, error_code)
};

}  // copy
}  // services
}  // ssf

MSGPACK_ADD_ENUM(ssf::services::copy::CopyRequestAck::Status);

#endif  // SSF_SERVICES_COPY_PACKET_CONTROL_H_

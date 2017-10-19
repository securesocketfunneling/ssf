#ifndef SSF_SERVICES_COPY_COPY_CONTEXT_H_
#define SSF_SERVICES_COPY_COPY_CONTEXT_H_

#include <fstream>
#include <functional>
#include <memory>
#include <mutex>

#include <boost/asio/io_service.hpp>

#include "common/crypto/sha1.h"
#include "common/filesystem/filesystem.h"

#include "services/copy/error_code.h"
#include "services/copy/i_copy_state.h"
#include "services/copy/packet.h"

namespace ssf {
namespace services {
namespace copy {

class CopyContext {
 public:
  using OnOutboundPacketFilled =
      std::function<void(const boost::system::error_code& ec)>;
  using OnStateChanged = std::function<void()>;
  using Hash = ssf::crypto::Sha1;

 private:
  using OnOutboundPacketFilledUPtr = std::unique_ptr<OnOutboundPacketFilled>;

 public:
  CopyContext(boost::asio::io_service& io_service);

  ~CopyContext();

  void Init(const std::string& i_input_filepath, bool check_file_integrity,
            bool i_is_stdin_input, uint64_t i_start_offset, bool i_resume,
            const std::string& i_output_dir,
            const std::string& i_output_filename);

  void Deinit();

  void set_on_state_changed(OnStateChanged on_state_changed) {
    on_state_changed_ = on_state_changed;
  }

  ssf::Path GetOutputFilepath() {
    ssf::Path result(output_dir);
    result /= output_filename;
    return result;
  }

  void AsyncFillOutboundPacket(Packet* packet, OnOutboundPacketFilled on_filled,
                               boost::system::error_code& ec);

  void FillOutboundPacket(boost::system::error_code& ec);

  void OnFillPacket(const boost::system::error_code& ec);

  void ProcessInboundPacket(const Packet& packet,
                            boost::system::error_code& ec);

  bool IsTerminal();

  bool IsClosed();

  boost::system::error_code GetErrorCode() const;

  void SetState(ICopyStateUPtr state);

 public:
  boost::asio::io_service& io_service_;
  std::ifstream input;
  std::ofstream output;
  std::string input_filepath;
  Hash::Digest input_file_digest;
  bool check_file_integrity;
  bool is_stdin_input;
  uint64_t start_offset;
  bool resume;
  std::string output_dir;
  std::string output_filename;
  Hash::Digest output_file_digest;
  ssf::Filesystem fs;
  ErrorCode error_code;

 private:
  std::recursive_mutex mutex_;
  ICopyStateUPtr state_;
  Packet* outbound_packet_;
  OnOutboundPacketFilledUPtr on_outbound_packet_filled_;
  OnStateChanged on_state_changed_;
};

using CopyContextUPtr = std::unique_ptr<CopyContext>;

}  // copy
}  // services
}  // ssf

#endif  // SSF_SERVICES_COPY_COPY_CONTEXT_H_

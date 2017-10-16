#include "services/copy/copy_context.h"

namespace ssf {
namespace services {
namespace copy {

CopyContext::CopyContext(boost::asio::io_service& io_service,
                         ICopyStateUPtr state)
    : io_service_(io_service),
      error_code(ErrorCode::kFailure),
      outbound_packet_(nullptr),
      state_(),
      on_state_changed_([]() {}) {
  SetState(std::move(state));
}

void CopyContext::Init(const std::string& i_input_filepath,
                       bool i_check_file_integrity, bool i_is_stdin_input,
                       uint64_t i_start_offset, bool i_resume,
                       const std::string& i_output_dir,
                       const std::string& i_output_filename) {
  input_filepath = i_input_filepath;
  check_file_integrity = i_check_file_integrity;
  is_stdin_input = i_is_stdin_input;
  start_offset = i_start_offset;
  resume = i_resume;
  output_dir = i_output_dir;
  output_filename = i_output_filename;
}

void CopyContext::AsyncFillOutboundPacket(Packet* packet,
                                          OnOutboundPacketFilled on_filled,
                                          boost::system::error_code& ec) {
  std::unique_lock<std::recursive_mutex> lock(mutex_);
  outbound_packet_ = packet;
  on_outbound_packet_filled_ =
      std::make_unique<OnOutboundPacketFilled>(on_filled);
  if (state_->FillOutboundPacket(this, outbound_packet_, ec)) {
    OnFillPacket(ec);
  }
}

void CopyContext::FillOutboundPacket(boost::system::error_code& ec) {
  std::unique_lock<std::recursive_mutex> lock(mutex_);
  if (!on_outbound_packet_filled_ || !outbound_packet_) {
    return;
  }

  if (state_->FillOutboundPacket(this, outbound_packet_, ec)) {
    OnFillPacket(ec);
  }
}

void CopyContext::OnFillPacket(const boost::system::error_code& ec) {
  std::unique_lock<std::recursive_mutex> lock(mutex_);
  if (!on_outbound_packet_filled_ || !outbound_packet_) {
    return;
  }

  (*on_outbound_packet_filled_)(ec);
  on_outbound_packet_filled_.reset();
  outbound_packet_ = nullptr;
}

void CopyContext::ProcessInboundPacket(const Packet& packet,
                                       boost::system::error_code& ec) {
  std::unique_lock<std::recursive_mutex> lock(mutex_);
  state_->ProcessInboundPacket(this, packet, ec);
}

bool CopyContext::IsTerminal() {
  std::unique_lock<std::recursive_mutex> lock(mutex_);
  return state_->IsTerminal(this);
}

bool CopyContext::IsClosed() {
  std::unique_lock<std::recursive_mutex> lock(mutex_);
  return state_->IsClosed(this);
}

boost::system::error_code CopyContext::GetErrorCode() const {
  return {error_code, get_copy_category()};
}

void CopyContext::SetState(ICopyStateUPtr state) {
  std::unique_lock<std::recursive_mutex> lock(mutex_);
  if (state_) {
    boost::system::error_code exit_ec;
    state_->Exit(this, exit_ec);
    state_.reset();
  }
  state_ = std::move(state);

  auto on_state_changed = [this]() {
    boost::system::error_code enter_ec;
    state_->Enter(this, enter_ec);
    on_state_changed_();
  };
  io_service_.post(on_state_changed);
}

void CopyContext::Deinit() {
  if (input.is_open()) {
    input.close();
  }
  if (output.is_open()) {
    output.close();
  }

  boost::system::error_code exit_ec;
  state_->Exit(this, exit_ec);

  on_outbound_packet_filled_.reset();
  on_state_changed_ = []() {};
}

}  // copy
}  // services
}  // ssf
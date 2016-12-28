#include "ssf/error/error.h"
#include "ssf/layer/proxy/socks4_strategy.h"
#include "ssf/log/log.h"

namespace ssf {
namespace layer {
namespace proxy {

Socks4Strategy::Socks4Strategy() : SocksStrategy(State::kConnecting) {}

void Socks4Strategy::Init(boost::system::error_code& ec) {
  set_state(State::kConnecting);
}

void Socks4Strategy::PopulateRequest(const std::string& target_host,
                                     uint16_t target_port, Buffer* p_request,
                                     uint32_t* p_expected_response_size,
                                     boost::system::error_code& ec) {
  using namespace ssf::network::socks;
  if (state() != State::kConnecting) {
    ec.assign(ssf::error::broken_pipe, ssf::error::get_ssf_category());
    return;
  }

  v4::Request socks_req;
  socks_req.Init(Request::Command::kConnect, target_host, target_port, ec);
  if (ec) {
    set_state(State::kError);
    return;
  }

  auto buffer_seq = socks_req.ConstBuffer();
  p_request->resize(boost::asio::buffer_size(buffer_seq));
  boost::asio::buffer_copy(boost::asio::buffer(*p_request), buffer_seq);
  *p_expected_response_size = 8;
}

void Socks4Strategy::ProcessResponse(const Buffer& response,
                                     boost::system::error_code& ec) {
  if (state() != State::kConnecting) {
    ec.assign(ssf::error::broken_pipe, ssf::error::get_ssf_category());
    return;
  }

  Reply socks_reply;
  boost::asio::buffer_copy(socks_reply.MutBuffer(),
                           boost::asio::buffer(response));
  if (socks_reply.null_byte() != 0) {
    SSF_LOG(kLogError)
        << "network[socks4 proxy]: connection failed (invalid socks reply)";
    set_state(State::kError);
    ec.assign(ssf::error::connection_refused, ssf::error::get_ssf_category());
    return;
  }
  if (socks_reply.status() != Reply::Status::kGranted) {
    SSF_LOG(kLogError) << "network[socks4 proxy]: connection failed (status "
                       << static_cast<uint32_t>(socks_reply.status()) << ")";
    set_state(State::kError);
    ec.assign(ssf::error::connection_refused, ssf::error::get_ssf_category());
    return;
  }

  set_state(State::kConnected);
  ec.assign(ssf::error::success, ssf::error::get_ssf_category());
}

}  // proxy
}  // layer
}  // ssf
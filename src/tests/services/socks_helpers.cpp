#include <ssf/network/socks/v4/request.h>
#include <ssf/network/socks/v4/reply.h>

#include <ssf/network/socks/v5/request_auth.h>
#include <ssf/network/socks/v5/reply_auth.h>
#include <ssf/network/socks/v5/request.h>
#include <ssf/network/socks/v5/reply.h>
#include <ssf/network/socks/v5/types.h>

#include <ssf/utils/enum.h>

#include "common/error/error.h"

#include "tests/services/socks_helpers.h"

namespace tests {
namespace socks {

SocksDummyClient::SocksDummyClient(const std::string& socks_server_addr,
                                   const std::string& socks_server_port,
                                   const std::string& target_addr,
                                   const std::string& target_port, size_t size)
    : io_service_(),
      p_worker_(new boost::asio::io_service::work(io_service_)),
      socket_(io_service_),
      socks_server_addr_(socks_server_addr),
      socks_server_port_(socks_server_port),
      target_addr_(target_addr),
      target_port_(target_port),
      size_(size) {}

bool SocksDummyClient::Init() {
  t_ = std::thread([&]() { io_service_.run(); });

  boost::asio::ip::tcp::resolver r(io_service_);
  boost::asio::ip::tcp::resolver::query q(socks_server_addr_,
                                          socks_server_port_);
  boost::system::error_code ec;
  boost::asio::connect(socket_, r.resolve(q), ec);

  if (ec) {
    SSF_LOG(kLogError) << "dummy client: fail to connect " << ec.value();
    Stop();
  }

  return !ec;
}

bool SocksDummyClient::ReceiveOneBuffer() {
  size_t received(0);

  while (received < size_) {
    boost::system::error_code ec;
    auto n = socket_.receive(boost::asio::buffer(one_buffer_), 0, ec);

    if (ec) {
      Stop();
      return false;
    }

    if (n == 0) {
      Stop();
      return false;
    } else {
      received += n;
      if (!CheckOneBuffer(n)) {
        Stop();
        return false;
      }
    }
  }

  Stop();
  return received == size_;
}

void SocksDummyClient::Stop() {
  boost::system::error_code ec;
  socket_.close(ec);

  p_worker_.reset();

  if (t_.joinable()) {
    t_.join();
  }
  io_service_.stop();
}

bool SocksDummyClient::CheckOneBuffer(size_t n) {
  for (size_t i = 0; i < n; ++i) {
    if (one_buffer_[i] != 1) {
      return false;
    }
  }

  return true;
}

Socks4DummyClient::Socks4DummyClient(const std::string& socks_server_addr,
                                     const std::string& socks_server_port,
                                     const std::string& target_addr,
                                     const std::string& target_port,
                                     size_t size)
    : SocksDummyClient(socks_server_addr, socks_server_port, target_addr,
                       target_port, size) {}

bool Socks4DummyClient::InitSocks() {
  using Request = ssf::network::socks::v4::Request;
  using Reply = ssf::network::socks::v4::Reply;

  boost::system::error_code ec;

  Request req;
  try {
    uint16_t port = static_cast<uint16_t>(std::stoul(target_port_));
    req.Init(Request::Command::kConnect, target_addr_, port, ec);
    if (ec) {
      Stop();
      return false;
    }
  } catch (const std::exception&) {
    Stop();
    return false;
  }
  boost::asio::write(socket_, req.ConstBuffer(), ec);
  if (ec) {
    SSF_LOG(kLogError) << "socks4 client: fail to write " << ec.value();
    Stop();
    return false;
  }

  Reply rep;
  boost::asio::read(socket_, rep.MutBuffer(), ec);
  if (ec) {
    SSF_LOG(kLogError) << "socks4 client: fail to read " << ec.value();
    Stop();
    return false;
  }
  if (rep.status() != Reply::Status::kGranted) {
    Stop();
    return false;
  }

  boost::asio::write(socket_, boost::asio::buffer(&size_, sizeof(size_t)), ec);

  if (ec) {
    SSF_LOG(kLogError) << "socks4 client: fail to write " << ec.value();
    Stop();
  }

  return !ec;
}

Socks5DummyClient::Socks5DummyClient(const std::string& socks_server_addr,
                                     const std::string& socks_server_port,
                                     const std::string& target_addr,
                                     const std::string& target_port,
                                     size_t size)
    : SocksDummyClient(socks_server_addr, socks_server_port, target_addr,
                       target_port, size) {}

bool Socks5DummyClient::InitSocks() {
  using AuthMethod = ssf::network::socks::v5::AuthMethod;
  using RequestAuth = ssf::network::socks::v5::RequestAuth;
  using ReplyAuth = ssf::network::socks::v5::ReplyAuth;

  using CommandType = ssf::network::socks::v5::CommandType;
  using AddressType = ssf::network::socks::v5::AddressType;
  using CommandStatus = ssf::network::socks::v5::CommandStatus;
  using Request = ssf::network::socks::v5::Request;
  using Reply = ssf::network::socks::v5::Reply;

  boost::system::error_code ec;

  RequestAuth auth_req;
  auth_req.Init({AuthMethod::kNoAuth,
                 AuthMethod::kGSSAPI,
                 AuthMethod::kUserPassword});
  boost::asio::write(socket_, auth_req.ConstBuffers(), ec);
  if (ec) {
    SSF_LOG(kLogError) << "socks5 client: fail to write auth request "
                       << ec.value();
    Stop();
    return false;
  }

  ReplyAuth auth_rep;
  boost::asio::read(socket_, auth_rep.MutBuffer(), ec);
  if (ec) {
    SSF_LOG(kLogError) << "socks5 client: fail to read auth reply"
                       << ec.value();
    Stop();
    return false;
  }
  if (auth_rep.auth_method() != ssf::ToIntegral(AuthMethod::kNoAuth)) {
    Stop();
    return false;
  }

  Request req;
  try {
    uint16_t port = static_cast<uint16_t>(std::stoul(target_port_));
    req.Init(target_addr_, port, ec);
    if (ec) {
      Stop();
      return false;
    }
  } catch (const std::exception&) {
    Stop();
    return false;
  }
  boost::asio::write(socket_, req.ConstBuffers(), ec);
  if (ec) {
    SSF_LOG(kLogError) << "socks5 client: fail to write conenct request"
                       << ec.value();
    Stop();
    return false;
  }

  Reply rep;
  boost::asio::read(socket_, rep.MutBaseBuffers(), ec);
  if (ec) {
    SSF_LOG(kLogError) << "socks5 client: fail to read reply (first part)"
                       << ec.value();
    Stop();
    return false;
  }
  boost::asio::read(socket_, rep.MutDynamicBuffers(), ec);
  if (ec) {
    SSF_LOG(kLogError) << "socks5 client: fail to read reply (second part)"
                       << ec.value();
    Stop();
    return false;
  }
  if (!rep.IsComplete() || !rep.AccessGranted()) {
    SSF_LOG(kLogError) << "socks5 client: request incomplete or access refused"
                       << ec.value();
    Stop();
    return false;
  }

  boost::asio::write(socket_, boost::asio::buffer(&size_, sizeof(size_t)), ec);
  if (ec) {
    SSF_LOG(kLogError) << "dummy client: fail to write " << ec.value();
    Stop();
  }

  return !ec;
}

}  // socks
}  // tests
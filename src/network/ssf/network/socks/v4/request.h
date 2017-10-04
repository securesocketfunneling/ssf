#ifndef SSF_NETWORK_SOCKS_V4_REQUEST_H_
#define SSF_NETWORK_SOCKS_V4_REQUEST_H_

#include <cstdint>
#include <array>
#include <memory>
#include <string>
#include <vector>

#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <boost/system/error_code.hpp>

namespace ssf {
namespace network {
namespace socks {
namespace v4 {

class Request {
 public:
  enum class Command : uint8_t { kConnect = 0x01, kBind = 0x02 };

 public:
  void Init(Command cmd, const std::string& addr, uint16_t port,
            boost::system::error_code& ec);

  uint8_t command() const { return command_; };

  std::string name() const { return name_; };

  std::string domain() const { return domain_; };

  uint16_t port() const;

  void set_name(const std::string& name) { name_ = name; }

  void set_domain(const std::string& domain) { domain_ = domain; }

  bool Is4aVersion() const;

  boost::asio::ip::tcp::endpoint Endpoint() const;

  std::array<boost::asio::mutable_buffer, 4> MutBuffer();

  std::vector<boost::asio::const_buffer> ConstBuffer() const;

 private:
  uint8_t version_;
  uint8_t command_;
  uint8_t port_high_byte_;
  uint8_t port_low_byte_;
  uint8_t null_byte_;
  boost::asio::ip::address_v4::bytes_type address_;
  std::string name_;
  std::string domain_;
};

}  // v4
}  // socks
}  // network
}  // ssf

#endif  // SSF_NETWORK_SOCKS_V4_REQUEST_H_

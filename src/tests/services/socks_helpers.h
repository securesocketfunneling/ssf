#ifndef TESTS_SERVICES_SOCKS_HELPERS_H_
#define TESTS_SERVICES_SOCKS_HELPERS_H_

#include <memory>

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/thread.hpp>

#include <ssf/log/log.h>

namespace tests {
namespace socks {

class Request {
 public:
  enum CommandType { kConnect = 0x01, kBind = 0x02 };

  Request(CommandType cmd, const boost::asio::ip::tcp::endpoint& endpoint,
          const std::string& user_id);

  std::array<boost::asio::const_buffer, 7> Buffers() const;

 private:
  unsigned char version_;
  unsigned char command_;
  unsigned char port_high_byte_;
  unsigned char port_low_byte_;
  boost::asio::ip::address_v4::bytes_type address_;
  std::string user_id_;
  unsigned char null_byte_;
};

class Reply {
 public:
  enum StatusType {
    kRequestGranted = 0x5a,
    kRequestFailed = 0x5b,
    kRequestFailedNoIdentd = 0x5c,
    kRequestFailedBadUserId = 0x5d
  };

  Reply();

  std::array<boost::asio::mutable_buffer, 5> Buffers();

  bool Success() const;

  unsigned char Status() const;

  boost::asio::ip::tcp::endpoint Endpoint() const;

 private:
  unsigned char null_byte_;
  unsigned char status_;
  unsigned char port_high_byte_;
  unsigned char port_low_byte_;
  boost::asio::ip::address_v4::bytes_type address_;
};

class DummyClient {
 public:
  DummyClient(const std::string& socks_server_addr,
              const std::string& socks_server_port,
              const std::string& target_addr, const std::string& target_port,
              size_t size);

  bool Init();

  bool InitSocks();

  bool ReceiveOneBuffer();

  void Stop();

 private:
  bool CheckOneBuffer(size_t n);

 private:
  boost::asio::io_service io_service_;
  std::unique_ptr<boost::asio::io_service::work> p_worker_;
  boost::asio::ip::tcp::socket socket_;
  boost::thread t_;
  std::string socks_server_addr_;
  std::string socks_server_port_;
  std::string target_addr_;
  std::string target_port_;
  std::size_t size_;
  std::array<uint8_t, 10240> one_buffer_;
};

}  // socks
}  // tests

#endif  // TESTS_SERVICES_SOCKS_HELPERS_H_